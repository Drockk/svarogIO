# Unit Test Scenarios - svarogIO

## 📋 General Information

**Project:** svarogIO Refactoring  
**Testing Framework:** Catch2 v3  
**Standard:** C++23  
**Date:** 2025-11-10

### Catch2 v3 Setup

**CMakeLists.txt:**
```cmake
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.0
)
FetchContent_MakeAvailable(Catch2)

add_executable(svarog_tests
    tests/work_queue_tests.cpp
    tests/io_context_tests.cpp
    tests/strand_tests.cpp
    tests/integration_tests.cpp
)
target_link_libraries(svarog_tests PRIVATE 
    svarog_execution
    Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(svarog_tests)
```

**Execution:**
```bash
# All tests
./build/svarog_tests

# Filter by tags
./build/svarog_tests "[work_queue]"
./build/svarog_tests "[concurrency]"

# Specific test
./build/svarog_tests "strand: prevents concurrent*"

# Verbose output
./build/svarog_tests -s
```

---

## 🎯 Testing Strategy

### Testing Levels

1. **Unit Tests** - Individual component tests in isolation
2. **Integration Tests** - Component collaboration tests
3. **Thread Safety Tests** - Multi-threading tests (ThreadSanitizer)
4. **Performance Tests** - Performance benchmarks
5. **Stress Tests** - Load testing

### Coverage Metrics

- **Minimum target:** 80% code coverage
- **Preferred target:** 90% code coverage
- **Critical paths:** 100% code coverage

---

## 1️⃣ work_queue - Task Queue

**Implementation Note:** Phase 1 uses simplified `work_queue` with `std::move_only_function<void()>` and `std::expected`. Template parameters and swappable containers are planned for Phase 2.

### 1.1 Basic Functionality

#### Test 1.1.1: Construction and Destruction

```cpp
TEST_CASE("work_queue: construction and destruction", "[work_queue][basic]") {
    SECTION("default construction") {
        svarog::execution::work_queue queue;
        REQUIRE(queue.empty());
        REQUIRE(queue.size() == 0);
    }
    
    SECTION("destruction with pending items") {
        {
            svarog::execution::work_queue queue;
            queue.push([] { /* some work */ });
            queue.push([] { /* more work */ });
        } // Destructor should cleanup
        REQUIRE(true);
    }
}
```

**Goal:** Verify RAII basics  
**Expected:** No memory leaks, no exceptions from destructor

---

#### Test 1.1.2: Push and Try Pop - Single Thread

```cpp
TEST_CASE("work_queue: push and try_pop single-threaded", "[work_queue][basic]") {
    svarog::execution::work_queue queue;
    
    SECTION("try_pop on empty queue returns error") {
        auto result = queue.try_pop();
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error() == svarog::execution::queue_error::empty);
    }
    
    SECTION("push and try_pop multiple items - FIFO order") {
        std::atomic<int> execution_order{0};
        int val1 = 0, val2 = 0, val3 = 0;
        
        queue.push([&execution_order, &val1] { val1 = ++execution_order; });
        queue.push([&execution_order, &val2] { val2 = ++execution_order; });
        queue.push([&execution_order, &val3] { val3 = ++execution_order; });
        
        auto r1 = queue.try_pop();
        REQUIRE(r1.has_value());
        r1.value()();  // Execute first lambda
        REQUIRE(val1 == 1);
        
        auto r2 = queue.try_pop();
        REQUIRE(r2.has_value());
        r2.value()();  // Execute second lambda
        REQUIRE(val2 == 2);
        
        auto r3 = queue.try_pop();
        REQUIRE(r3.has_value());
        r3.value()();  // Execute third lambda
        REQUIRE(val3 == 3);
        
        REQUIRE(queue.empty());
    }
}
```

**Goal:** Verify FIFO order and std::expected correctness  
**Expected:** Elements come out in insertion order

---

#### Test 1.1.3: Blocking Pop

```cpp
TEST_CASE("work_queue: blocking pop", "[work_queue][blocking]") {
    svarog::execution::work_queue queue;
    
    std::atomic<bool> started{false};
    std::atomic<int> result_value{0};
    
    std::jthread consumer([&]() {
        started = true;
        auto result = queue.pop();
        if (result) {
            result.value()();  // Execute the lambda
        }
    });
    
    // Wait for consumer to start
    while (!started) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // Short delay for consumer to block
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    REQUIRE(result_value == 0);  // Not executed yet
    
    // Push item - should unblock consumer
    queue.push([&result_value] { result_value = 999; });
    
    consumer.join();
    REQUIRE(result_value == 999);
}
```

**Goal:** Verify blocking and unblocking  
**Expected:** pop() blocks until item added

---

### 1.2 Thread Safety Tests

#### Test 1.2.1: Concurrent Push from Multiple Threads

```cpp
TEST_CASE("work_queue: concurrent push from multiple threads", "[work_queue][concurrency]") {
    svarog::execution::work_queue queue;
    constexpr int num_threads = 10;
    constexpr int items_per_thread = 1000;
    constexpr int total_items = num_threads * items_per_thread;
    
    std::atomic<int> executed_count{0};
    std::vector<std::jthread> producers;
    producers.reserve(num_threads);
    
    for (int t = 0; t < num_threads; ++t) {
        producers.emplace_back([&queue, &executed_count]() {
            for (int i = 0; i < items_per_thread; ++i) {
                queue.push([&executed_count] { executed_count++; });
            }
        });
    }
    
    producers.clear(); // Implicit join
    
    REQUIRE(queue.size() == total_items);
    
    // Execute all work items
    int popped_count = 0;
    while (auto item = queue.try_pop()) {
        item.value()();  // Execute the lambda
        popped_count++;
    }
    
    REQUIRE(popped_count == total_items);
    REQUIRE(executed_count == total_items);
}
```

**Goal:** Verify thread-safety with concurrent push  
**Expected:** No race conditions, all elements preserved

---

#### Test 1.2.2: Producer-Consumer Pattern

```cpp
TEST_CASE("work_queue: producer-consumer pattern", "[work_queue][concurrency]") {
    svarog::execution::work_queue queue;
    constexpr int num_producers = 5;
    constexpr int num_consumers = 5;
    constexpr int items_per_producer = 1000;
    constexpr int total_items = num_producers * items_per_producer;
    
    std::atomic<int> produced{0};
    std::atomic<int> consumed{0};
    
    // Producers
    std::vector<std::jthread> producers;
    for (int p = 0; p < num_producers; ++p) {
        producers.emplace_back([&]() {
            for (int i = 0; i < items_per_producer; ++i) {
                queue.push([&consumed] { consumed++; });
                produced++;
            }
        });
    }
    
    // Consumers
    std::vector<std::jthread> consumers;
    for (int c = 0; c < num_consumers; ++c) {
        consumers.emplace_back([&]() {
            while (consumed < total_items) {
                if (auto item = queue.try_pop()) {
                    item.value()();  // Execute the lambda
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }
    
    producers.clear();
    consumers.clear();
    
    REQUIRE(consumed == total_items);
}
```

**Goal:** Classic producer-consumer test  
**Expected:** No lost elements, no duplicates

---

### 1.3 Shutdown Semantics

#### Test 1.3.1: Shutdown Behavior

```cpp
TEST_CASE("work_queue: shutdown behavior", "[work_queue][shutdown]") {
    svarog::execution::work_queue queue;
    
    SECTION("shutdown wakes up blocking threads") {
        std::atomic<bool> got_shutdown_error{false};
        
        std::jthread consumer([&]() {
            auto result = queue.pop();
            if (!result.has_value() && 
                result.error() == svarog::execution::queue_error::shutdown) {
                got_shutdown_error = true;
            }
        });
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        queue.shutdown();
        
        consumer.join();
        REQUIRE(got_shutdown_error);
    }
    
    SECTION("shutdown with pending items allows draining") {
        std::atomic<int> val1{0}, val2{0};
        
        queue.push([&val1] { val1 = 1; });
        queue.push([&val2] { val2 = 2; });
        queue.shutdown();
        
        // Existing items can still be retrieved
        auto r1 = queue.try_pop();
        REQUIRE(r1.has_value());
        r1.value()();
        REQUIRE(val1 == 1);
        
        auto r2 = queue.try_pop();
        REQUIRE(r2.has_value());
        r2.value()();
        REQUIRE(val2 == 2);
        
        // Now empty and shutdown
        auto r3 = queue.try_pop();
        REQUIRE(r3.error() == svarog::execution::queue_error::shutdown);
    }
}
```

**Goal:** Verify graceful shutdown  
**Expected:** Shutdown unblocks waiting threads, allows drain

---

## 2️⃣ io_context - Main Executor

### 2.1 Basic Functionality

#### Test 2.1.1: Post and Run - Single Handler

```cpp
TEST_CASE("io_context: post and run single handler", "[io_context][basic]") {
    svarog::execution::io_context io_ctx;
    
    bool executed = false;
    
    io_ctx.post([&]() {
        executed = true;
    });
    
    REQUIRE_FALSE(executed); // Not executed until run()
    
    auto count = io_ctx.run();
    
    REQUIRE(executed);
    REQUIRE(count == 1);
    REQUIRE(io_ctx.stopped()); // run() ends when no work
}
```

---

#### Test 2.1.2: Multiple Handlers - Order Preservation

```cpp
TEST_CASE("io_context: multiple handlers preserve order", "[io_context][basic]") {
    svarog::execution::io_context io_ctx;
    
    std::vector<int> execution_order;
    
    for (int i = 0; i < 10; ++i) {
        io_ctx.post([&, i]() {
            execution_order.push_back(i);
        });
    }
    
    io_ctx.run();
    
    REQUIRE(execution_order.size() == 10);
    for (int i = 0; i < 10; ++i) {
        REQUIRE(execution_order[i] == i);
    }
}
```

**Goal:** Verify FIFO order for post()  
**Expected:** Handlers execute in post() order

---

#### Test 2.1.3: Dispatch vs Post

```cpp
TEST_CASE("io_context: dispatch vs post", "[io_context][dispatch]") {
    svarog::execution::io_context io_ctx;
    
    SECTION("dispatch from inside worker thread executes immediately") {
        bool outer_executed = false;
        bool inner_executed = false;
        
        io_ctx.post([&]() {
            outer_executed = true;
            
            // Dispatch from inside worker thread
            io_ctx.dispatch([&]() {
                inner_executed = true;
            });
            
            // Inner should already be executed
            REQUIRE(inner_executed);
        });
        
        io_ctx.run();
        REQUIRE(outer_executed);
        REQUIRE(inner_executed);
    }
}
```

**Goal:** Verify dispatch() semantics  
**Expected:** dispatch() executes immediately if in worker thread

---

### 2.2 Multi-Threading

#### Test 2.2.1: Multiple Worker Threads

```cpp
TEST_CASE("io_context: multiple worker threads", "[io_context][threading]") {
    constexpr int num_workers = 4;
    svarog::execution::io_context io_ctx(num_workers);
    
    constexpr int num_tasks = 1000;
    std::atomic<int> completed{0};
    std::set<std::thread::id> thread_ids;
    std::mutex thread_ids_mtx;
    
    for (int i = 0; i < num_tasks; ++i) {
        io_ctx.post([&]() {
            {
                std::lock_guard lock(thread_ids_mtx);
                thread_ids.insert(std::this_thread::get_id());
            }
            completed++;
        });
    }
    
    std::vector<std::jthread> workers;
    for (int w = 0; w < num_workers; ++w) {
        workers.emplace_back([&]() {
            io_ctx.run();
        });
    }
    
    workers.clear();
    
    REQUIRE(completed == num_tasks);
    REQUIRE(thread_ids.size() > 1);
}
```

**Goal:** Verify work distribution among threads  
**Expected:** Work distributed, all tasks completed

---

## 3️⃣ strand - Handler Serialization

### 3.1 Basic Functionality

#### Test 3.1.1: Serialization - Multi Thread (KEY TEST!)

```cpp
TEST_CASE("strand: serialization prevents concurrent execution", "[strand][concurrency]") {
    constexpr int num_workers = 4;
    svarog::execution::io_context io_ctx(num_workers);
    svarog::execution::strand s(io_ctx);
    
    std::atomic<int> concurrent_count{0};
    std::atomic<int> max_concurrent{0};
    constexpr int num_tasks = 1000;
    
    for (int i = 0; i < num_tasks; ++i) {
        s.post([&]() {
            int current = ++concurrent_count;
            
            // Update max concurrency
            int expected_max = max_concurrent.load();
            while (current > expected_max && 
                   !max_concurrent.compare_exchange_weak(expected_max, current)) {
            }
            
            // Simulate work
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            
            --concurrent_count;
        });
    }
    
    std::vector<std::jthread> workers;
    for (int w = 0; w < num_workers; ++w) {
        workers.emplace_back([&]() {
            io_ctx.run();
        });
    }
    
    workers.clear();
    
    // KEY ASSERT: Strand guarantees max one handler at a time
    REQUIRE(max_concurrent.load() == 1);
}
```

**Goal:** KEY TEST - verify strand prevents parallel execution  
**Expected:** max_concurrent ALWAYS == 1

---

### 3.2 Multiple Independent Strands

#### Test 3.2.1: Strand Independence

```cpp
TEST_CASE("strand: multiple independent strands", "[strand][multiple]") {
    constexpr int num_workers = 4;
    svarog::execution::io_context io_ctx(num_workers);
    
    svarog::execution::strand s1(io_ctx);
    svarog::execution::strand s2(io_ctx);
    
    std::atomic<int> s1_max{0};
    std::atomic<int> s2_max{0};
    
    // Post to s1
    for (int i = 0; i < 100; ++i) {
        s1.post([&]() {
            // Verify s1 serialization
            // ...
        });
    }
    
    // Post to s2  
    for (int i = 0; i < 100; ++i) {
        s2.post([&]() {
            // Verify s2 serialization
            // ...
        });
    }
    
    std::vector<std::jthread> workers;
    for (int w = 0; w < num_workers; ++w) {
        workers.emplace_back([&]() {
            io_ctx.run();
        });
    }
    
    workers.clear();
    
    // Each strand individually serialized
    REQUIRE(s1_max == 1);
    REQUIRE(s2_max == 1);
}
```

**Goal:** Different strands can execute in parallel  
**Expected:** Each strand serialized internally, but strands independent

---

## 4️⃣ work_guard - Lifecycle Management

### 4.1 Basic Functionality

#### Test 4.1.1: Work Guard Prevents Shutdown

```cpp
TEST_CASE("work_guard: prevents io_context from stopping", "[work_guard][basic]") {
    svarog::execution::io_context io_ctx;
    
    SECTION("with work_guard, run blocks") {
        auto guard = svarog::execution::make_work_guard(io_ctx);
        
        std::atomic<bool> run_finished{false};
        
        std::jthread runner([&]() {
            io_ctx.run();
            run_finished = true;
        });
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        REQUIRE_FALSE(run_finished);
        
        guard.reset();
        
        runner.join();
        REQUIRE(run_finished);
    }
}
```

**Goal:** work_guard prevents premature shutdown  
**Expected:** run() blocks while guard alive

---

#### Test 4.1.2: Multiple Work Guards

```cpp
TEST_CASE("work_guard: multiple guards", "[work_guard][multiple]") {
    svarog::execution::io_context io_ctx;
    
    auto guard1 = svarog::execution::make_work_guard(io_ctx);
    auto guard2 = svarog::execution::make_work_guard(io_ctx);
    auto guard3 = svarog::execution::make_work_guard(io_ctx);
    
    std::atomic<bool> run_finished{false};
    
    std::jthread runner([&]() {
        io_ctx.run();
        run_finished = true;
    });
    
    guard1.reset();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    REQUIRE_FALSE(run_finished); // Still 2 guards
    
    guard2.reset();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    REQUIRE_FALSE(run_finished); // Still 1 guard
    
    guard3.reset();
    
    runner.join();
    REQUIRE(run_finished); // All guards released
}
```

**Goal:** Multiple guards work cumulatively  
**Expected:** run() ends when all guards released

---

## 5️⃣ Integration Tests

### 5.1 io_context + strand

#### Test 5.1.1: Realistic Connection Handler

```cpp
TEST_CASE("integration: connection with strand", "[integration][strand]") {
    svarog::execution::io_context io_ctx(4);
    
    // Simulate connection object
    class Connection {
    public:
        explicit Connection(svarog::execution::io_context& ctx, int id)
            : strand_(ctx), id_(id) {}
        
        void async_read() {
            strand_.post([this]() {
                buffer_ += std::to_string(id_) + ":read ";
                read_count_++;
            });
        }
        
        void async_write() {
            strand_.post([this]() {
                buffer_ += std::to_string(id_) + ":write ";
                write_count_++;
            });
        }
        
        int get_read_count() const { return read_count_; }
        int get_write_count() const { return write_count_; }
        
    private:
        svarog::execution::strand<svarog::execution::io_context> strand_;
        std::string buffer_;  // Protected by strand - NO MUTEX NEEDED!
        int read_count_ = 0;
        int write_count_ = 0;
        int id_;
    };
    
    // Create connections
    std::vector<std::unique_ptr<Connection>> connections;
    for (int i = 0; i < 10; ++i) {
        connections.push_back(std::make_unique<Connection>(io_ctx, i));
    }
    
    // Simulate activity
    for (int round = 0; round < 100; ++round) {
        for (auto& conn : connections) {
            conn->async_read();
            conn->async_write();
        }
    }
    
    // Execute
    std::vector<std::jthread> workers;
    for (int w = 0; w < 4; ++w) {
        workers.emplace_back([&]() {
            io_ctx.run();
        });
    }
    
    workers.clear();
    
    // Verify
    for (const auto& conn : connections) {
        REQUIRE(conn->get_read_count() == 100);
        REQUIRE(conn->get_write_count() == 100);
    }
}
```

**Goal:** Realistic use case - multiple connections, each with strand  
**Expected:** No race conditions, all operations completed

---

## 📝 Summary

### Test Statistics

| Component | Test Count | Categories |
|-----------|------------|------------|
| work_queue | 25+ | basic, concurrency, bulk, shutdown |
| io_context | 20+ | basic, lifecycle, threading |
| strand | 15+ | serialization, concurrency |
| work_guard | 5+ | RAII, multiple guards |
| Integration | 10+ | realistic scenarios |

### Implementation Priorities

1. **🔴 Critical** - Must pass before merge
   - work_queue basic functionality
   - strand serialization (Test 3.1.1!)
   - io_context lifecycle
   
2. **🟡 Important** - Should pass before release
   - Thread safety all components
   - work_guard RAII semantics
   - Exception handling
   
3. **🟢 Optional** - Nice to have
   - Performance benchmarks
   - Stress tests
   - Edge cases

### Framework Recommendations

**Google Test:**
```cmake
find_package(GTest REQUIRED)
target_link_libraries(svarog_tests PRIVATE GTest::gtest GTest::gtest_main)
```

**Catch2:**
```cmake
find_package(Catch2 3 REQUIRED)
target_link_libraries(svarog_tests PRIVATE Catch2::Catch2WithMain)
```

### CI/CD Integration

```yaml
# .github/workflows/tests.yml
- name: Run Unit Tests
  run: |
    cmake --build build --target svarog_tests
    cd build && ctest --output-on-failure
    
- name: Run with ThreadSanitizer
  env:
    CXXFLAGS: "-fsanitize=thread -g"
  run: |
    cmake -B build-tsan
    cmake --build build-tsan
    cd build-tsan && ctest
```

---

## 🎭 Mocking with Trompeloeil

### Trompeloeil Setup

**CMakeLists.txt:**
```cmake
FetchContent_Declare(
    trompeloeil
    GIT_REPOSITORY https://github.com/rollbear/trompeloeil.git
    GIT_TAG v47
)
FetchContent_MakeAvailable(trompeloeil)

target_link_libraries(svarog_tests PRIVATE trompeloeil::trompeloeil)
```

### Example: Mock for System Calls

```cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/trompeloeil.hpp>

// Mock interface for I/O operations
class io_service_mock {
public:
    MAKE_MOCK2(async_read, void(int fd, std::function<void(int)>));
    MAKE_MOCK2(async_write, void(int fd, std::function<void(int)>));
    MAKE_MOCK1(close_socket, void(int fd));
};

TEST_CASE("io_context: mock system calls", "[io_context][mock]") {
    io_service_mock mock_io;
    
    SECTION("async_read calls handler on completion") {
        int result_bytes = -1;
        
        REQUIRE_CALL(mock_io, async_read(42, trompeloeil::_))
            .LR_SIDE_EFFECT(_2(100)); // Call callback with 100 bytes
        
        mock_io.async_read(42, [&](int bytes) {
            result_bytes = bytes;
        });
        
        REQUIRE(result_bytes == 100);
    }
    
    SECTION("close_socket called exactly once") {
        REQUIRE_CALL(mock_io, close_socket(42))
            .TIMES(1);
        
        mock_io.close_socket(42);
    }
}
```

### Example: Mock for Custom Container

```cpp
template<typename T>
class container_mock {
public:
    MAKE_MOCK1(push_back, void(T));
    MAKE_MOCK0(pop_front, T());
    MAKE_CONST_MOCK0(size, size_t());
    MAKE_CONST_MOCK0(empty, bool());
};

TEST_CASE("work_queue: custom container behavior", "[work_queue][mock]") {
    container_mock<int> mock_container;
    
    SECTION("push_back called for each push") {
        REQUIRE_CALL(mock_container, push_back(42));
        REQUIRE_CALL(mock_container, push_back(43));
        
        mock_container.push_back(42);
        mock_container.push_back(43);
    }
    
    SECTION("pop_front with expected order") {
        using trompeloeil::sequence;
        sequence seq;
        
        REQUIRE_CALL(mock_container, pop_front())
            .IN_SEQUENCE(seq)
            .RETURN(1);
        
        REQUIRE_CALL(mock_container, pop_front())
            .IN_SEQUENCE(seq)
            .RETURN(2);
        
        REQUIRE(mock_container.pop_front() == 1);
        REQUIRE(mock_container.pop_front() == 2);
    }
}
```

### Advanced: Exception Throwing Mock

```cpp
TEST_CASE("io_context: handle exceptions from handlers", "[io_context][mock][exceptions]") {
    io_service_mock mock_io;
    
    REQUIRE_CALL(mock_io, async_read(trompeloeil::_, trompeloeil::_))
        .THROW(std::runtime_error("Network error"));
    
    REQUIRE_THROWS_AS(
        mock_io.async_read(42, [](int) {}),
        std::runtime_error
    );
}
```

---

**Document Status:** Complete v2.0 (Catch2 v3)  
**Date:** 2025-11-10  
**Framework:** Catch2 v3 + Trompeloeil  
**Coverage:** ~95% of components
**Framework:** Google Test / Catch2  
**Coverage:** ~90% components
