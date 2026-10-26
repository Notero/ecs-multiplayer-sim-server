#include <chrono>
#include "SparseSet.hpp"
#include "UIDGenerator.hpp"
#include <iostream>

struct Position{
    double x,y;
};

struct Velocity{
    double x,y;
};


SparseSet<Velocity> velComp(500000);
SparseSet<Position> posComp(500000);
std::chrono::duration<double> accumulator(0.0);
std::chrono::duration<double> fixedDt(1.0 / 60.0);

void tick(){
    for(auto& c: velComp){
        Position& pos = posComp.getComponent(c.first);
        Velocity vel = c.second;
        pos.x += vel.x * fixedDt.count();
        pos.y += vel.y * fixedDt.count();

    }
}

int main()
{
    UIDGenerator uidgen;

    //tree
    for (size_t i = 0; i < 20000; i++)
    {
        auto tree = uidgen.getUID();
        posComp.insert(std::pair(tree,Position{5,5}));
    }

    //balls
    for (size_t i = 0; i < 50000; i++)
    {
        auto ball = uidgen.getUID();
        velComp.insert(std::pair(ball,Velocity{1,1}));
        posComp.insert(std::pair(ball,Position{10,10}));
    }
    
    
    



    auto prev = std::chrono::steady_clock::now();
    int tickCount = 0;
    bool prog = true;
    while(prog){
        auto curr = std::chrono::steady_clock::now();
        std::chrono::duration<double> frameTime = curr - prev;
        prev = curr;
        accumulator += frameTime;
        while (accumulator >= fixedDt)
        {
            tick();
            tickCount++;
            accumulator -= fixedDt;
            if (tickCount % 1200 == 0) {
                prog = false;
            }
        }
    }
    
    return 0;
}
