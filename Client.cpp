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
#include <chrono>
#include <csignal>
#include "Connection.hpp"
#include "Pub.hpp"
#include "Structs.hpp"
#include "ORingBuffer.hpp"

std::atomic<bool> shutdownReq = false;
using TimePoint = std::chrono::steady_clock::time_point;

struct Snapshot{
    std::vector<std::pair<size_t, Position>> snapshot;
    TimePoint timestamp = std::chrono::steady_clock::now();
};

void reciever(Connection& con, size_t& ID,std::vector<size_t>& eID, std::mutex& mtx, ORingBuffer<Snapshot>& snaps){
    
    sigset_t a;
    sigemptyset(&a);
    sigaddset(&a, SIGINT);
    int pth = pthread_sigmask(SIG_BLOCK,&a,nullptr);
    if(0 != pth){
        std::cerr << std::strerror(pth) << std::endl;
        return;
    }

    sockaddr_in sip{};
    socklen_t slen = sizeof(sip);
    sockaddr* sp = reinterpret_cast<sockaddr*>(&sip);
    
    unsigned int tickCount = 0;
    while (!shutdownReq)
    {
        std::string response;
        con.recvUDP(sp,slen,response);

        UType type;

        std::memcpy(&type,response.data(),sizeof(type));
        
        switch (type)
        {
        case UType::Nugde:
            {
                Ping react{UType::React,ID};
                std::string_view view(reinterpret_cast<const char*>(&react), sizeof(react));
                con.sendUDP(sp, slen, view);
            }
            break;
        case UType::Snapshot:
            {
                // Later, on the receiving side:
                size_t count;
                std::memcpy(&count, response.data() + sizeof(UType), sizeof(count));
                std::vector<std::pair<size_t, Position>> rv(count);
                std::memcpy(rv.data(), response.data() + sizeof(count) + sizeof(UType), count * sizeof(std::pair<size_t, Position>));
                Snapshot s{rv};
                {
                    std::lock_guard lock(mtx);
                    snaps.push(s);
                }

                tickCount++;
                if (tickCount % 10 == 0) 
                {
                    for(auto& r: rv){
                        std::cout << "entity id==" << r.first << " | Position x" << r.second.x << " y" << r.second.y << std::endl;
                    }
                }
            }
            break;
        case UType::Spawn:
            {
                Ping spawn{};
                std::memcpy(&spawn,response.data(),sizeof(spawn));
                {
                    std::lock_guard lock(mtx);
                    eID.push_back(spawn.id);
                }
            }
            break;
        default:
            break;
        }


    }
}
//signal for option+c or shutdown
void onSigint(int) {
    shutdownReq = true;
}

int main()
{
    size_t ID;
    std::vector<size_t> eID;
    std::vector<std::thread> curr_active_threads;
    std::mutex mtx;

    ORingBuffer<Snapshot> snapshots(10);

    //signal connection SIGINT means Ctrl + c or option + c in macos
    struct sigaction sa{};          // zero it
    sa.sa_handler = onSigint;       // which function to call
    sa.sa_flags = 0;                // 0 = no SA_RESTART, so accept gets interrupted
    if(sigaction(SIGINT, &sa, nullptr) == -1){
        perror("sigaction");
        return 1;
    }

    auto client = std::make_unique<Connection>(socket(AF_INET, SOCK_DGRAM, 0));

    if (client->get_fd() == -1)
    {
        perror("socket");
        return 1;
    }

    //bind options
    sockaddr_in dest{};
    //AF_INET address family internet ipv4
    //AF_INET6 address family internet ipv6
    dest.sin_family = AF_INET;
    //htons: host to network byte order (Mac is little-endian, network is big-endian)
    dest.sin_port = htons(3300);
    //htonl: same for 32-bit (long) values
    dest.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    socklen_t len = sizeof(dest);
    sockaddr* sp = reinterpret_cast<sockaddr*>(&dest);

    Ping c{UType::Connect};
    std::string_view view(reinterpret_cast<const char*>(&c), sizeof(c));
    client->sendUDP(sp,len,view);
    
    std::string conRes;
    sockaddr_in empty{};
    socklen_t esize = sizeof(empty);
    sockaddr* ssp = reinterpret_cast<sockaddr*>(&empty);
    client->recvUDP(ssp,esize,conRes);
    Ping z{};
    std::memcpy(&z,conRes.data(),sizeof(z));
    
    ID = z.id;

    std::thread t(reciever, std::ref(*client),std::ref(ID), std::ref(eID), std::ref(mtx), std::ref(snapshots));
    curr_active_threads.push_back(std::move(t));


    while (!shutdownReq)
    {
        std::cout << "Hello |" << std::endl;
        std::cout << "-------" << std::endl;
        std::cout << "1. Spawn Ball, 2. Add Speed to A ball, 3. Disconnect" << std::endl;
        int a;
        std::cin >> a;
        switch (a)
        {
        case 1:
            {
                Ping p{UType::Spawn, ID};
                std::string_view v(reinterpret_cast<const char*>(&p),sizeof(p));
                client->sendUDP(sp,len,v);
            }
            break;
        case 2:
            {
                std::cout << "ID list for owned balls" << std::endl;
                {
                    std::lock_guard lock(mtx);
                    for(auto& entity: eID){
                        std::cout << entity << std::endl;
                    }
                }
                double x,y;
                size_t entityID;
                std::cout << "-----------------------------------" << std::endl;
                std::cout << "Enter entity ID from the list above" << std::endl;
                std::cin >> entityID;
                std::cout << "Enter speed towards x" << std::endl;
                std::cin >> x;
                std::cout << "Enter speed towards y" << std::endl;
                std::cin >> y;
                Input i{UType::Input, Velocity{x,y}, ID, entityID};
                std::string_view v(reinterpret_cast<const char*>(&i),sizeof(i));
                client->sendUDP(sp, len, v);
            }
            break;
        case 3:
            {
                shutdownReq.store(true);
            }
            break;        
        default:
            std::cout << "Wrong Input" << std::endl;
            break;
        }
    }

    Ping d{UType::Disconnect, ID};
    std::string_view dview(reinterpret_cast<const char*>(&d), sizeof(d));
    client->sendUDP(sp,len,dview);

    shutdown(client->get_fd(), SHUT_RDWR);
    //thread safety comment out
    for(auto& thr : curr_active_threads){
        thr.join();
    }

    return 0;
}