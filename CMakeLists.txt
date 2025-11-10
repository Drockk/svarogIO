cmake_minimum_required(VERSION 3.23)

include(cmake/CPM.cmake)

project("svarogIO")

CPMAddPackage("gh:microsoft/GSL@4.2.0")

set(CMAKE_CXX_STANDARD 23)

enable_testing()

add_subdirectory("examples")
add_subdirectory("svarog")
add_subdirectory("tests")

