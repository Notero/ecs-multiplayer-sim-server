add_executable(localsim localsim.cpp)
add_executable(mserver MultiplayerServer.cpp)
add_executable(mclient Launcher.cpp)
add_executable(swarm Swarm.cpp)

target_link_libraries(mclient PRIVATE SparseSet UIDGenerator Connection Sim St ORingBuffer Client)
target_link_libraries(localsim PRIVATE SparseSet UIDGenerator)
target_link_libraries(mserver PRIVATE SparseSet UIDGenerator Connection Sim St)
target_link_libraries(swarm PRIVATE Client)