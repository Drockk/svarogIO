cmake_minimum_required(VERSION 3.23)

include(cmake/CPM.cmake)

project("svarogIO")

CPMAddPackage("gh:microsoft/GSL@4.2.0")
CPMAddPackage("gh:catchorg/Catch2@3.11.0")

set(CMAKE_CXX_STANDARD 23)

# Contract programming option
option(SVAROG_ENABLE_CONTRACTS_IN_RELEASE "Enable contract checks in Release builds" OFF)

if(SVAROG_ENABLE_CONTRACTS_IN_RELEASE)
    add_compile_definitions(SVAROG_ENABLE_CONTRACTS_IN_RELEASE)
endif()

enable_testing()

add_subdirectory("examples")
add_subdirectory("svarog")
add_subdirectory("tests")

