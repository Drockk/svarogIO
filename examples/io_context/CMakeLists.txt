cmake_minimum_required(VERSION 3.25)

add_executable(io_context_example main.cpp)
target_link_libraries(io_context_example PRIVATE svarog)
target_compile_features(io_context_example PRIVATE cxx_std_23)
