cmake_minimum_required(VERSION 3.25)

add_executable(thread_pool_example main.cpp)
target_link_libraries(thread_pool_example PRIVATE svarog)
target_compile_features(thread_pool_example PRIVATE cxx_std_23)
