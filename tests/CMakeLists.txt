# Unit tests for svarog project

# Include test helper functions
include(${CMAKE_SOURCE_DIR}/cmake/SvarogTestHelpers.cmake)

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
add_svarog_test(execution/thread_pool_tests.cpp)
add_svarog_test(execution/strand_tests.cpp)
add_svarog_test(execution/coroutine_tests.cpp)
add_svarog_test(io/io_context_tests.cpp)

# ============================================================================
# Future: Add more test modules here
# ============================================================================
# add_svarog_test(network/socket_tests.cpp)
# add_svarog_test(task/scheduler_tests.cpp)
