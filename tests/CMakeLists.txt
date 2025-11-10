add_executable(contracts_test
    core/contracts_test.cpp
)

target_link_libraries(contracts_test
    PRIVATE
    svarog
    Microsoft.GSL::GSL
)

add_test(NAME ContractsTest COMMAND contracts_test)
