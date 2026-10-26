add_executable(localsim localsim.cpp)

target_link_libraries(localsim PRIVATE SparseSet)
target_link_libraries(localsim PRIVATE UIDGenerator)