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
#include "Connection.hpp"
#include "Pub.hpp"
#include "Structs.hpp"

std::atomic<bool> shutdownReq = false;

void reciever(Connection& con){
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

    while (!shutdownReq)
    {
        std::string response;
        con.recvUDP(sp,slen,response);

        UType type;

        std::memcpy(&type,&response,sizeof(type));
        
        switch (type)
        {
        case UType::Nugde:
            
            break;
        case UType::Snapshot:
            break;
        case UType::Spawn:
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

int main(){
    size_t ID;
    std::vector<size_t> eID;
    std::vector<std::thread> curr_active_threads;

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



    while (!shutdownReq)
    {

    }

    Ping d{UType::Disconnect, ID};
    std::string_view dview(reinterpret_cast<const char*>(&d), sizeof(d));
    client->sendUDP(sp,len,dview);

    //thread safety comment out
    for(auto& thr : curr_active_threads){
        thr.join();
    }

    return 0;
}