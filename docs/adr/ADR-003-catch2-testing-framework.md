# ADR-003: Catch2 v3 Testing Framework Selection

**Date:** 2025-11-10  
**Status:** Accepted

## Context

Project requires comprehensive test suite covering:
- Unit tests (work_queue, io_context, strand)
- Integration tests (realistic async I/O scenarios)
- Thread safety tests (concurrency, race conditions)
- Performance tests (benchmarks)

We need a testing framework meeting these requirements:

1. **Modern C++:** Full C++23 compatibility
2. **BDD style:** Given/When/Then for async test readability
3. **Hierarchical tests:** Lifecycle test organization (start/stop/restart)
4. **Mocking support:** For component isolation
5. **Benchmarking:** Built-in performance testing
6. **CMake integration:** FetchContent, CTest discovery
7. **Maintenance:** Active development, large community

### Candidates

1. **Google Test (gtest/gmock)** - Industry standard
2. **Catch2 v3** - Modern C++ testing framework
3. **Boost.Test** - Part of Boost ecosystem
4. **doctest** - Lightweight alternative

## Decision

**We choose Catch2 v3 as the primary testing framework.**

### Rationale

#### 1. BDD Style - Perfect for Async Code

**Catch2:**
```cpp
TEST_CASE("io_context: lifecycle", "[io_context][lifecycle]") {
    svarog::execution::io_context io_ctx;
    
    SECTION("stop prevents run from blocking") {
        io_ctx.stop();
        REQUIRE(io_ctx.stopped());
        auto count = io_ctx.run();
        REQUIRE(count == 0);
    }
    
    SECTION("restart after stop") {
        io_ctx.stop();
        io_ctx.restart();
        REQUIRE_FALSE(io_ctx.stopped());
    }
}
```

**Advantage:** `SECTION` allows shared setup and clear scenario hierarchy.

#### 2. Natural Hierarchical Organization

```cpp
TEST_CASE("work_queue: shutdown scenarios", "[work_queue][shutdown]") {
    work_queue<int> queue;
    
    SECTION("graceful shutdown") {
        SECTION("with empty queue") {
            queue.shutdown();
            REQUIRE(queue.stopped());
        }
        
        SECTION("with pending items") {
            queue.push(1);
            queue.shutdown();
            REQUIRE(queue.try_pop().has_value());
        }
    }
}
```

Each `SECTION` = independent test case with own setup/teardown, sharing common initialization.

#### 3. Built-in Benchmarking

```cpp
TEST_CASE("work_queue: push throughput", "[work_queue][bench]") {
    work_queue<int> queue;
    
    BENCHMARK("push single item") {
        queue.push(42);
    };
}
```

**Bonus:** Statistics (mean, median, std dev) out-of-the-box.

#### 4. Header-Only (Optional)

- Zero dependencies beyond C++ stdlib
- Easy setup: `FetchContent` in CMake
- Fast compilation with precompiled headers

#### 5. Integration with Mocking: Trompeloeil

Catch2 officially supports **Trompeloeil** (header-only C++ mocking):

```cpp
#include <catch2/trompeloeil.hpp>

class io_service_mock {
public:
    MAKE_MOCK2(async_read, void(int fd, std::function<void(int)>));
};

TEST_CASE("mock system calls", "[mock]") {
    io_service_mock mock_io;
    
    REQUIRE_CALL(mock_io, async_read(42, trompeloeil::_))
        .LR_SIDE_EFFECT(_2(100));
    
    mock_io.async_read(42, [](int bytes) {
        REQUIRE(bytes == 100);
    });
}
```

**Advantage vs gmock:** Header-only, zero dependencies.

## Consequences

### Positive

1. **Readability:** BDD style = self-documenting tests
2. **Developer Experience:** Natural C++ syntax, excellent error messages
3. **Maintenance:** Active development (v3.5.0 - 2024), large community (11k+ stars)
4. **Integration:** CMake `catch_discover_tests()`, CTest native support
5. **Performance:** Built-in benchmarking = no need for Google Benchmark

### Negative (Trade-offs)

1. **Learning Curve:** Developers accustomed to gtest need training
   - **Mitigation:** Documentation in UNIT_TEST_SCENARIOS.md
2. **Ecosystem:** Google Test has larger ecosystem
   - **Mitigation:** Trompeloeil covers mocking, Catch2 benchmarking sufficient
3. **Compilation Time:** Header-only = each TU compiles Catch2
   - **Mitigation:** Catch2WithMain as precompiled library

## Alternatives

### 1. Google Test + gmock
**Rejected - reasons:**
- No `SECTION` = code duplication for lifecycle tests
- gmock syntax more verbose than Trompeloeil
- No built-in benchmarking
- **However:** Good fallback if Catch2 fails

### 2. Boost.Test
**Rejected - reasons:**
- Dependency on entire Boost (too large footprint)
- Less modern C++ features vs Catch2

### 3. doctest
**Rejected - reasons:**
- Younger project (less stability)
- No built-in benchmarking

## Critical Example: Strand Serialization Test

```cpp
TEST_CASE("strand: prevents concurrent execution", "[strand][concurrency][critical]") {
    svarog::execution::io_context io_ctx(4);
    svarog::execution::strand s(io_ctx);
    
    std::atomic<int> concurrent{0};
    std::atomic<int> max_concurrent{0};
    
    SECTION("single strand serializes handlers") {
        for (int i = 0; i < 1000; ++i) {
            s.post([&]() {
                int current = ++concurrent;
                int expected = max_concurrent.load();
                while (current > expected && 
                       !max_concurrent.compare_exchange_weak(expected, current)) {}
                
                std::this_thread::sleep_for(std::chrono::microseconds(10));
                --concurrent;
            });
        }
        
        std::vector<std::jthread> workers;
        for (int w = 0; w < 4; ++w) {
            workers.emplace_back([&]() { io_ctx.run(); });
        }
        workers.clear();
        
        REQUIRE(max_concurrent == 1); // CRITICAL: Must be 1
    }
}
```

**Why Catch2 better:** Natural flow (setup → section → assertion), easy to add more sections.

## References

- [Catch2 Documentation](https://github.com/catchorg/Catch2/tree/devel/docs)
- [Trompeloeil Documentation](https://github.com/rollbear/trompeloeil)
- UNIT_TEST_SCENARIOS_EN.md - Test implementation in Catch2 v3

## Related Decisions

- [ADR-001](ADR-001-boost-asio-architecture-en.md) - Async architecture requires extensive testing
- BENCHMARK_SCENARIOS_EN.md - Using Catch2 benchmarking

---

**Approved by:** drock  
**Implementation Status:** Adopted in test scenarios
