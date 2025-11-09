add_subdirectory("simple_tcp_client")
add_subdirectory("simple_tcp_server")

add_custom_target("examples")
add_dependencies("examples"
    "simple_tcp_client"
    "simple_tcp_server"
)
