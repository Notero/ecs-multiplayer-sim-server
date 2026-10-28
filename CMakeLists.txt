add_executable(localsim localsim.cpp)
add_executable(mserver MultiplayerServer.cpp)
add_executable(mclient Client.cpp)

target_link_libraries(mclient PRIVATE SparseSet UIDGenerator Connection Sim St ORingBuffer)
target_link_libraries(localsim PRIVATE SparseSet UIDGenerator)
target_link_libraries(mserver PRIVATE SparseSet UIDGenerator Connection Sim St)