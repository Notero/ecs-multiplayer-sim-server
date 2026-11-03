cmake_minimum_required(VERSION 3.16)
project(ecs_multiplayer_sim_server CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(library)

add_executable(localsim localsim.cpp)
add_executable(mserver MultiplayerServer.cpp)
add_executable(mclient Launcher.cpp)
add_executable(swarm Swarm.cpp)

target_link_libraries(mclient PRIVATE SparseSet UIDGenerator Connection Sim St ORingBuffer Client)
target_link_libraries(localsim PRIVATE SparseSet UIDGenerator)
target_link_libraries(mserver PRIVATE SparseSet UIDGenerator Connection Sim St)
target_link_libraries(swarm PRIVATE Client)
