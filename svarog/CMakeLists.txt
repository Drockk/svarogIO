
add_library("svarog" SHARED)

target_sources(
  "svarog"
  PRIVATE
    "source/svarog/network/Socket.cpp"
  PUBLIC
    FILE_SET svarog_headers
    TYPE HEADERS
    BASE_DIRS "include"
    FILES
      "include/svarog/network/Socket.hpp"
)
