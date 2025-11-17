cmake_minimum_required(VERSION 3.25)

add_executable(work_queue_example main.cpp)
target_link_libraries(work_queue_example PRIVATE svarog)
target_compile_features(work_queue_example PRIVATE cxx_std_23)
