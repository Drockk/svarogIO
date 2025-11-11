#Unit tests for svarog project

add_subdirectory("core")

# Contracts Unit Tests
add_executable(svarog_tests
    execution/execution_context_tests.cpp
)

target_link_libraries(svarog_tests
    PRIVATE
    svarog
    Catch2::Catch2WithMain
)

add_test(NAME SvarogTests COMMAND svarog_tests)
