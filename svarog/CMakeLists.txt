add_library("svarog" SHARED)

target_sources(
  "svarog"
  PRIVATE
    "source/svarog/execution/strand.cpp"
    "source/svarog/execution/thread_pool.cpp"
    "source/svarog/execution/work_guard.cpp"
    "source/svarog/io/detail/epoll_reactor.cpp"
    "source/svarog/io/io_context.cpp"
    "source/svarog/ip/ip_address.cpp"
    "source/svarog/ip/ip_address_v4.cpp"
    "source/svarog/network/socket.cpp"
  PUBLIC
    FILE_SET svarog_headers
    TYPE HEADERS
    BASE_DIRS "include"
    FILES
      "include/svarog/core/contracts.hpp"
      "include/svarog/execution/strand.hpp"
      "include/svarog/execution/thread_pool.hpp"
      "include/svarog/execution/work_queue.hpp"
      "include/svarog/execution/work_guard.hpp"
      "include/svarog/io/detail/epoll_reactor.hpp"
      "include/svarog/io/detail/platform_config.hpp"
      "include/svarog/io/detail/reactor_interface.hpp"
      "include/svarog/io/io_context.hpp"
      "include/svarog/ip/ip_address.hpp"
      "include/svarog/ip/ip_address_v4.hpp"
      "include/svarog/network/socket.hpp"
)

target_link_libraries("svarog"
PUBLIC
Microsoft.GSL::GSL
tl::expected
)

if(WIN32)
    target_link_libraries(svarog PRIVATE ws2_32 mswsock)
endif()
