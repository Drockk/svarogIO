
add_executable("simple_tcp_client")

target_sources(
  "simple_tcp_client"
  PRIVATE
    "main.cpp"
)

target_link_libraries(
  "simple_tcp_client"
  PRIVATE
    "svarog"
)
