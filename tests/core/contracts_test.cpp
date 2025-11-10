// Test file for contract macros
#include <svarog/core/contracts.hpp>
#include <iostream>

using namespace svarog::core;

void test_expects_pass() {
    int value = 42;
    SVAROG_EXPECTS(value > 0);  // Should pass
    std::cout << "✓ SVAROG_EXPECTS with valid condition passed\n";
}

void test_ensures_pass() {
    int result = 100;
    SVAROG_ENSURES(result >= 0);  // Should pass
    std::cout << "✓ SVAROG_ENSURES with valid condition passed\n";
}

// This test should fail in Debug builds
// Uncomment to test failure handling
/*
void test_expects_fail() {
    int value = -1;
    SVAROG_EXPECTS(value > 0);  // Should fail in Debug
}
*/

int main() {
    std::cout << "Testing SVAROG contract macros...\n";
    
    #ifdef NDEBUG
    std::cout << "Running in RELEASE mode (contracts disabled)\n";
    #else
    std::cout << "Running in DEBUG mode (contracts enabled)\n";
    #endif
    
    test_expects_pass();
    test_ensures_pass();
    
    std::cout << "\n✓ All contract tests passed!\n";
    
    // Uncomment to test failure:
    // test_expects_fail();
    
    return 0;
}
