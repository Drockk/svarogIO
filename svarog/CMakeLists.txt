add_library("svarog" SHARED)

target_sources(
  "svarog"
  PRIVATE
    "source/svarog/execution/strand.cpp"
    "source/svarog/execution/thread_pool.cpp"
    "source/svarog/execution/work_guard.cpp"
    "source/svarog/io/detail/timer_queue.cpp"
    "source/svarog/io/io_context.cpp"
    "source/svarog/network/ip/address_v4.cpp"
    "source/svarog/network/ip/address_v6.cpp"
    "source/svarog/network/ip/address.cpp"
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
      "include/svarog/io/detail/iocp_reactor.hpp"
      "include/svarog/io/detail/kqueue_reactor.hpp"
      "include/svarog/io/detail/select_reactor.hpp"
      "include/svarog/io/detail/timer_queue.hpp"
      "include/svarog/io/detail/platform_config.hpp"
      "include/svarog/io/detail/reactor_interface.hpp"
      "include/svarog/io/io_context.hpp"
      "include/svarog/io/steady_timer.hpp"
      "include/svarog/io/system_timer.hpp"
      "include/svarog/network/detail/socket_types.hpp"
      "include/svarog/network/ip/address_v4.hpp"
      "include/svarog/network/ip/address_v6.hpp"
      "include/svarog/network/ip/address.hpp"
      "include/svarog/network/ip/endpoint.hpp"
      "include/svarog/network/ip/tcp.hpp"
      "include/svarog/network/ip/udp.hpp"
      "include/svarog/network/socket.hpp"
)

target_link_libraries("svarog"
PUBLIC
Microsoft.GSL::GSL
tl::expected
)

if(APPLE OR BSD)
    target_sources(svarog PRIVATE "source/svarog/io/detail/kqueue_reactor.cpp")
endif()

if(LINUX)
    target_sources(svarog PRIVATE "source/svarog/io/detail/epoll_reactor.cpp")
endif()

if(WIN32)
    target_sources(svarog PRIVATE "source/svarog/io/detail/iocp_reactor.cpp")
    target_link_libraries(svarog PRIVATE ws2_32 mswsock)
endif()
