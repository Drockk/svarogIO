cmake_minimum_required(VERSION 3.25)

add_executable(simple_coroutine main.cpp)
target_link_libraries(simple_coroutine PRIVATE svarog)
target_compile_features(simple_coroutine PRIVATE cxx_std_23)
