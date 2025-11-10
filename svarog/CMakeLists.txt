add_library("svarog" SHARED)

target_sources(
  "svarog"
  PRIVATE
    "source/svarog/ip/ip_address.cpp"
    "source/svarog/ip/ip_address_v4.cpp"
    "source/svarog/network/socket.cpp"
    "source/svarog/task/task.cpp"
  PUBLIC
    FILE_SET svarog_headers
    TYPE HEADERS
    BASE_DIRS "include"
    FILES
      "include/svarog/core/contracts.hpp"
      "include/svarog/ip/ip_address.hpp"
      "include/svarog/ip/ip_address_v4.hpp"
      "include/svarog/network/socket.hpp"
      "include/svarog/task/task.hpp"
)

target_link_libraries("svarog"
  Microsoft.GSL::GSL
)
