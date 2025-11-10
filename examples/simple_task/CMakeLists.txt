
add_executable("simple_task")

set(CMAKE_CXX_STANDARD 23)

target_sources(
  "simple_task"
  PRIVATE
    "main.cpp"
)

target_link_libraries(
  "simple_task"
  PRIVATE
    "svarog"
)
