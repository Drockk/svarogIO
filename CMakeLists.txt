cmake_minimum_required(VERSION 3.23)

include(cmake/CPM.cmake)

project("svarogIO")

CPMAddPackage("gh:microsoft/GSL@4.2.0")
CPMAddPackage("gh:catchorg/Catch2@3.11.0")

# Contract programming option
option(SVAROG_ENABLE_CONTRACTS_IN_RELEASE "Enable contract checks in Release builds" OFF)

if(SVAROG_ENABLE_CONTRACTS_IN_RELEASE)
    add_compile_definitions(SVAROG_ENABLE_CONTRACTS_IN_RELEASE)
endif()

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Common warnings for both GCC and Clang
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        -Wconversion
        -Wsign-conversion
        -Wdouble-promotion
        -Wformat=2
    )
    
    # GCC-specific warnings
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(
            -Wmisleading-indentation
            -Wduplicated-cond
            -Wduplicated-branches
            -Wlogical-op
            -Wnull-dereference
            -Wuseless-cast
            -Wimplicit-fallthrough
        )
        
        # GCC 13+ specific warnings
        if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13.0)
            add_compile_options(
                -Wdangling-reference
                -Warith-conversion
            )
        endif()
        
        # Additional GCC warnings
        add_compile_options(
            -Wextra-semi
            -Wsuggest-override
            -Wsuggest-final-types
            -Wsuggest-final-methods
            -Wctor-dtor-privacy
            -Wstack-protector
            -Wtrampolines
            -Wdisabled-optimization
        )
    endif()
    
    # Clang-specific warnings
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        add_compile_options(
            -Wextra-semi
            -Wsuggest-override
        )
    endif()
endif()

enable_testing()

add_subdirectory("examples")
add_subdirectory("svarog")
add_subdirectory("tests")

