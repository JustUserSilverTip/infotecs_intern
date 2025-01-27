add_executable(program_client main.cpp)
target_link_libraries(program_client functions)
target_include_directories(program_client PRIVATE ../lib)