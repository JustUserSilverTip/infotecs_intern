add_executable(program_server main.cpp)
target_link_libraries(program_server functions)
target_include_directories(program_server PRIVATE ../lib)