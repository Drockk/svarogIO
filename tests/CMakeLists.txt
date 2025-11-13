# Unit tests for svarog project

# Helper function to create a test executable from a single cpp file
# 
# Each test file gets its own executable and CTest registration.
# This allows running tests individually and provides better isolation.
#
# Usage: add_svarog_test(execution/work_queue_basic_tests.cpp)
#
# Benefits:
# - Fast incremental builds (only rebuild changed test)
# - Can run single test: ./build/release/tests/work_queue_basic_tests
# - Better parallel execution in CTest
# - Easier debugging (smaller binary, clear test name)
#
# The function automatically:
# - Creates executable named after the cpp file (without extension)
# - Links with svarog library and Catch2::Catch2WithMain
# - Registers test with CTest
function(add_svarog_test TEST_SOURCE)
    # Get filename without extension for test name
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    
    # Create executable
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    
    # Link with svarog and Catch2
    target_link_libraries(${TEST_NAME}
        PRIVATE
        svarog
        Catch2::Catch2WithMain
    )
    
    # Register with CTest
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

# Core tests (uses custom main, not Catch2)
add_subdirectory("core")

# ============================================================================
# Execution Module Tests
# ============================================================================
# Each .cpp file becomes a separate test executable.
# To add a new test:
#   1. Create tests/execution/my_new_test.cpp
#   2. Add line: add_svarog_test(execution/my_new_test.cpp)
#   3. Rebuild - test will automatically appear in CTest

add_svarog_test(execution/execution_context_tests.cpp)
add_svarog_test(execution/work_queue_basic_tests.cpp)

# ============================================================================
# Future: Add more test modules here
# ============================================================================
# add_svarog_test(network/socket_tests.cpp)
# add_svarog_test(task/scheduler_tests.cpp)
