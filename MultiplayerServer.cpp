#include <sys/socket.h>   // socket, bind, listen, accept, setsockopt, shutdown
#include <netinet/in.h>   // sockaddr_in, INADDR_ANY, htons/htonl
#include <arpa/inet.h>    // inet_pton, inet_ntop (IP string <-> binary)
#include <iostream>
#include <thread>
#include <vector>
#include <functional>
#include <mutex>
#include <memory>
#include <atomic>
#include <cstdio>
#include <pthread.h>
#include <format>
#include <csignal>
#include <chrono>
#include "Pub.hpp"
#include "Connection.hpp"
#include "SparseSet.hpp"
#include "UIDGenerator.hpp"
#include "Simulation.hpp"
#include "Structs.hpp"



std::atomic<bool> shutdownReq = false;
const int on = 1;
//signal for option+c or shutdown
void onSigint(int) {
    shutdownReq = true;
}

void sim(Simulation& ballSim, std::function<void()>& serveraction){

    sigset_t a;
    sigemptyset(&a);
    sigaddset(&a, SIGINT);
    int pth = pthread_sigmask(SIG_BLOCK,&a,nullptr);
    if(0 != pth){
        std::cerr << std::strerror(pth) << std::endl;
        return;
    }

    ballSim.accumulatorLoop(shutdownReq,std::optional<std::function<void()>>(serveraction));
}

void keepAlive(SparseSet<sockaddr_in>& clients, const Connection& con, std::mutex& mtx){
    
    sigset_t a;
    sigemptyset(&a);
    sigaddset(&a, SIGINT);
    int pth = pthread_sigmask(SIG_BLOCK,&a,nullptr);
    if(0 != pth){
        std::cerr << std::strerror(pth) << std::endl;
        return;
    }

    while(!shutdownReq){
        std::this_thread::sleep_for(std::chrono::seconds(20));
        {
            std::lock_guard lock(mtx);
            for(auto& a: clients){
                const sockaddr* sp = reinterpret_cast<sockaddr*>(&a.second);
                socklen_t len = sizeof(a.second);
                Ping nudge{UType::Nugde, a.first};
                std::string_view view(reinterpret_cast<const char*>(&nudge), sizeof(nudge));
                con.sendUDP(sp,len,view);
            }
        }
    }
}

int main()
{
    Pub gamingPub(10);
    Simulation ballSim;
    UIDGenerator servergen;
    SparseSet<sockaddr_in> clients(1000);
    std::vector<std::thread> curr_active_threads;
    std::mutex mtx;
    
    //signal connection SIGINT means Ctrl + c or option + c in macos
    struct sigaction sa{};          // zero it
    sa.sa_handler = onSigint;       // which function to call
    sa.sa_flags = 0;                // 0 = no SA_RESTART, so accept gets interrupted
    if(sigaction(SIGINT, &sa, nullptr) == -1){
        perror("sigaction");
        return 1;
    }

    //SOCK_DGRAM == UDP
    auto server = std::make_unique<Connection>(socket(AF_INET, SOCK_DGRAM, 0));

    if (server->get_fd() == -1)
    {
        perror("socket");
        return 1;
    }

    if (setsockopt(server->get_fd(),SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on)) == -1){
        perror("setsockopt");
        return 1;
    }

    //bind options
    sockaddr_in serverAddrIn{};
    //AF_INET address family internet ipv4
    //AF_INET6 address family internet ipv6
    serverAddrIn.sin_family = AF_INET;
    //htons: host to network byte order (Mac is little-endian, network is big-endian)
    serverAddrIn.sin_port = htons(3300);
    //htonl: same for 32-bit (long) values
    serverAddrIn.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    const sockaddr* sp = reinterpret_cast<sockaddr*>(&serverAddrIn);
    //bind a name/address to the socket
    if (bind(server->get_fd(), sp, sizeof(serverAddrIn)) == -1)
    {
        perror("bind");
        return 1;
    }

    std::function broadcast = [&ballSim, &clients, &server, &mtx]() mutable{
        const auto snap = ballSim.snapshot();
        using SnapValue = decltype(snap)::value_type;

        std::string broadcastmsg;
        broadcastmsg.resize(sizeof(UType) + sizeof(size_t) + snap.size() * sizeof(SnapValue));
        size_t snapSize = snap.size();
        UType t = UType::Snapshot;
        std::memcpy(broadcastmsg.data(), &t, sizeof(UType));
        //copies snapsize into data
        std::memcpy(broadcastmsg.data() + sizeof(UType), &snapSize, sizeof(snapSize));
        //copy each element into broadcast
        std::memcpy(broadcastmsg.data() + sizeof(UType) + sizeof(snapSize), snap.data(), snap.size() * sizeof(SnapValue));

        {
            std::lock_guard lock(mtx);
            for(auto& c : clients){
            sockaddr* sp = reinterpret_cast<sockaddr*>(&c.second);
            server->sendUDP(sp, sizeof(c.second),broadcastmsg);
            }
        }
    };

    std::thread t(keepAlive,std::ref(clients), std::ref(*server), std::ref(mtx));
    curr_active_threads.push_back(std::move(t));
    std::thread c(sim, std::ref(ballSim), std::ref(broadcast));
    curr_active_threads.push_back(std::move(c));

    while (!shutdownReq)
    {
        sockaddr_in client{};
        socklen_t clientLen = sizeof(client);
        sockaddr* sp = reinterpret_cast<sockaddr*>(&client);
        std::string response;
        server->recvUDP(sp,clientLen,response);

        //peaks
        UType type;
        std::memcpy(&type, response.data(), sizeof(UType));

        switch (type)
        {
        case UType::Connect:
            {
                //mutable is needed copy of a client needed but lambda always captures const
                //mutable makes them not const ?
                std::function job = [&servergen,&clients,client,clientLen,&server,&mtx]() mutable
                {
                    sockaddr* sp = reinterpret_cast<sockaddr*>(&client);
                    //generate a unique id to client and add it to clients set
                    size_t clientId;
                    {
                        std::lock_guard lock(mtx);
                        clientId = servergen.getUID();
                        clients.insert(std::pair<size_t,sockaddr_in>(clientId,client));
                    }
                    
                    //send the id back to client
                    Ping c{UType::Connect,clientId};
                    std::string_view view(reinterpret_cast<const char*>(&c), sizeof(c));
                    server->sendUDP(sp, clientLen,view);
                };
                gamingPub.addJob(job);
            }
            break;
        case UType::Disconnect:
            {
                std::function job = [response,&clients,&mtx,client,&server,clientLen,&ballSim]() mutable
                {
                    sockaddr* sp = reinterpret_cast<sockaddr*>(&client);
                    Ping ping;
                    std::memcpy(&ping, response.data(), sizeof(ping));
                    {
                        std::lock_guard lock(mtx);
                        auto comp = clients.getComponent(ping.id);
                        if (comp.sin_port == client.sin_port && comp.sin_addr.s_addr == client.sin_addr.s_addr) 
                        {
                            clients.extract(ping.id);
                            ballSim.killAll(ping.id);
                            Ping c{UType::Disconnect, ping.id};
                            std::string_view view(reinterpret_cast<const char*>(&c),sizeof(c));
                            server->sendUDP(sp, clientLen,view);
                        }
                    }
                };
                gamingPub.addJob(job);
            }
            break;
        case UType::Spawn:
            {
                std::function job = [response,&clients,client,&server,clientLen,&ballSim,&mtx]() mutable 
                {
                    std::lock_guard lock(mtx);
                    sockaddr* sp = reinterpret_cast<sockaddr*>(&client); 
                    Ping ping;
                    std::memcpy(&ping, response.data(), sizeof(ping));
                    auto comp = clients.getComponent(ping.id);
                    if (comp.sin_port == client.sin_port && comp.sin_addr.s_addr == client.sin_addr.s_addr) 
                        {
                            auto a = ballSim.spawn(ping.id);
                            Ping c{UType::Success, a};
                            std::string_view view(reinterpret_cast<const char*>(&c),sizeof(c));
                            server->sendUDP(sp, clientLen,view);
                        }
                    
                };
                gamingPub.addJob(job);
            }
            break;
        case UType::Input:
            {
                std::function job = [response,&clients,client,&server,clientLen,&ballSim,&mtx]() mutable {
                    std::lock_guard lock(mtx);
                    sockaddr* sp = reinterpret_cast<sockaddr*>(&client);
                    Input input;
                    std::memcpy(&input, response.data(),sizeof(input));
                    auto comp = clients.getComponent(input.cID);
                    if (comp.sin_port == client.sin_port && comp.sin_addr.s_addr == client.sin_addr.s_addr) 
                        {
                            ballSim.inputSpeed(input.eID, static_cast<const Velocity&>(input.v));
                            Ping c{UType::Success, input.eID};
                            std::string_view view(reinterpret_cast<const char*>(&c),sizeof(c));
                            server->sendUDP(sp, clientLen,view);
                        }
                };
                gamingPub.addJob(job);
            }
            break;
        case UType::React:
            {
                std::function job = [response,&clients,&mtx,client,&server,clientLen]() mutable 
                {
                    sockaddr* sp = reinterpret_cast<sockaddr*>(&client);
                    Ping react;
                    std::memcpy(&react,response.data(),sizeof(react));
                    {
                        std::lock_guard lock(mtx);
                        auto comp = clients.getComponent(react.id);
                        if (comp.sin_port == client.sin_port && comp.sin_addr.s_addr == client.sin_addr.s_addr) 
                        {   
                            Ping c{UType::Success, react.id};
                            std::string_view view(reinterpret_cast<const char*>(&c),sizeof(c));
                            server->sendUDP(sp, clientLen,view);
                        }
                    }
                };
                gamingPub.addJob(job);
            }
            break;
        default:
            break;
        }
    }
    
    
    
    //thread safety comment out
    for(auto& thr : curr_active_threads){
        thr.join();
    }

    return 0;
}