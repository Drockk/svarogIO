cmake_minimum_required(VERSION 3.25)

add_executable(work_guard_example main.cpp)
target_link_libraries(work_guard_example PRIVATE svarog)
target_compile_features(work_guard_example PRIVATE cxx_std_23)
