# Contracts Unit Tests
add_executable(contracts_test
    contracts_test.cpp
)

target_link_libraries(contracts_test
    PRIVATE
    svarog
)

add_test(NAME ContractsTest COMMAND contracts_test)