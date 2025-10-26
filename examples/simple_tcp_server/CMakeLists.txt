
add_executable("simple_tcp_server")

target_sources(
  "simple_tcp_server"
  PRIVATE
    "main.cpp"
)

target_link_libraries(
  "simple_tcp_server"
  PRIVATE
    "svarog"
)
