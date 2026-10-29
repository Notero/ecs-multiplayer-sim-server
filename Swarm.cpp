#include "Client.hpp"
#include <memory>
#include <string>

int main(int argc, char const *argv[])
{
    size_t clientCount = std::stoi(argv[1]);
    size_t roundCount = std::stoi(argv[2]);

    std::vector<std::unique_ptr<Client>> simc;
    
    for (size_t i = 0; i < clientCount; i++)
    {
        simc.emplace_back(std::make_unique<Client>());
    }

    for(auto& c: simc){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        c->connect();
        std::cout << c->getID() << std::endl;
        c->startReceiver();
    }

    for(auto& c: simc){
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        c->spawnReq();
    }
    size_t rounds = 0;
    while(true){
    if(rounds == roundCount){
        break;
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
        for(auto& c: simc){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            auto e = c->getOwned().front();
            c->speedReq(1.0,1.0,e);
        }
    rounds++;
    }
    shutdownReq.store(true);
    return 0;
}
