add_executable(localsim localsim.cpp)
add_executable(mserver MultiplayerServer.cpp)

target_link_libraries(localsim PRIVATE SparseSet UIDGenerator)
target_link_libraries(mserver PRIVATE SparseSet UIDGenerator Connection Sim St)