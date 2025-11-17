add_subdirectory(simple_task)
add_subdirectory(simple_tcp_client)
add_subdirectory(simple_tcp_server)
add_subdirectory(simple_coroutine)

# Core component examples
add_subdirectory(execution_context)
add_subdirectory(work_queue)
add_subdirectory(work_guard)
add_subdirectory(io_context)
add_subdirectory(thread_pool)

add_custom_target("examples")
add_dependencies("examples"
    "simple_task"
    "simple_tcp_client"
    "simple_tcp_server"
    "simple_coroutine"
    "execution_context_example"
    "work_queue_example"
    "work_guard_example"
    "io_context_example"
    "thread_pool_example"
)
