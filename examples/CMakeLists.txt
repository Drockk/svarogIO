add_subdirectory(simple_tcp_client)
add_subdirectory(simple_tcp_server)

# Core component examples
add_subdirectory(work_queue)
add_subdirectory(work_guard)
add_subdirectory(io_context)
add_subdirectory(thread_pool)
add_subdirectory(strand_example)

add_custom_target("examples")
add_dependencies("examples"
    "simple_tcp_client"
    "simple_tcp_server"
    "work_queue_example"
    "work_guard_example"
    "io_context_example"
    "thread_pool_example"
    "strand_example"
)
