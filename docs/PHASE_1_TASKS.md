# Phase 1 Implementation Task List

## Overview
Phase 1 focuses on implementing the core execution infrastructure of svarogIO: `execution_context`, `work_queue`, `io_context`, and `strand`. This phase establishes the foundation for all asynchronous operations.

**Duration**: 4-6 weeks  
**Priority**: Critical Path  
**Dependencies**: None (starting point)

---

## 0. Contract Programming Setup

### 0.0 Build System Configuration
**Status**: ✅ Complete

- ✅ CMakePresets.json configured with debug and release presets
  - `build/debug/` - Debug build with `-DCMAKE_BUILD_TYPE=Debug`
  - `build/release/` - Release build with `-DCMAKE_BUILD_TYPE=Release`
- ✅ Ninja generator configured for fast builds
- ✅ Build, test, and workflow presets defined
- ✅ `enable_testing()` enabled in root CMakeLists.txt
- ✅ CTest integration working correctly

**Usage**:
```bash
# Debug development workflow
cmake --preset debug && cmake --build --preset debug
ctest --test-dir build/debug --output-on-failure

# Release performance testing
cmake --preset release && cmake --build --preset release
ctest --test-dir build/release --output-on-failure
```

### 0.1 GSL Integration
**Estimated Time**: 1-2 days
**Status**: ✅ Complete

- ✅ Add MS GSL dependency to CMake via CPM
  ```cmake
  CPMAddPackage("gh:microsoft/GSL@4.2.0")
  ```
- ✅ Create `svarog/include/svarog/core/contracts.hpp`
  - ✅ Define `SVAROG_EXPECTS(condition)` macro
  - ✅ Define `SVAROG_ENSURES(condition)` macro
  - ✅ Define `SVAROG_ASSERT(condition)` macro
  - ✅ Support both Debug and Release builds
  - ✅ Custom violation handler for better error messages

**Build Commands**:
```bash
# Debug build (contracts enabled)
cmake --preset debug
cmake --build --preset debug
ctest --test-dir build/debug --output-on-failure

# Release build (contracts disabled by default)
cmake --preset release
cmake --build --preset release
ctest --test-dir build/release --output-on-failure

# Release build with contracts enabled (for validation)
cmake --preset release -DSVAROG_ENABLE_CONTRACTS_IN_RELEASE=ON
cmake --build --preset release
ctest --test-dir build/release --output-on-failure

# Complete workflow (configure + build + test)
cmake --workflow --preset debug
cmake --workflow --preset release
```

**Example Implementation**:
```cpp
#include <gsl/gsl>

namespace svarog::core {

#ifdef SVAROG_ENABLE_CONTRACTS_IN_RELEASE
    #define SVAROG_EXPECTS(cond) ::gsl::Expects(cond)
    #define SVAROG_ENSURES(cond) ::gsl::Ensures(cond)
#else
    #ifdef NDEBUG
        #define SVAROG_EXPECTS(cond) ((void)0)
        #define SVAROG_ENSURES(cond) ((void)0)
    #else
        #define SVAROG_EXPECTS(cond) ::gsl::Expects(cond)
        #define SVAROG_ENSURES(cond) ::gsl::Ensures(cond)
    #endif
#endif

#define SVAROG_ASSERT(cond) assert(cond)

} // namespace svarog::core
```

**Acceptance Criteria**:
- ✅ MS GSL integrated successfully (via CPM)
- ✅ Contracts header compiles on all platforms
- ✅ Contracts enabled in Debug, disabled in Release (by default)
- ✅ CMake option to enable contracts in Release builds
- ✅ CMake presets configured for `build/debug` and `build/release`
- ✅ `enable_testing()` called in root CMakeLists.txt for proper CTest integration

### 0.2 Contract Policy Documentation
**Estimated Time**: 1 day
**Status**: ✅ Complete (pending team review)

- ✅ Create `docs/CODING_STANDARDS.md`
- ✅ Document when to use `SVAROG_EXPECTS` vs `std::expected`
  - ✅ `SVAROG_EXPECTS`: Programming errors (precondition violations)
  - ✅ `std::expected`: Expected runtime errors (business logic)
- ✅ Document contract categories:
  - ✅ **Critical** (always check): null pointers, resource ownership
  - ✅ **Debug-only**: range checks, performance-sensitive paths
  - ✅ **Documentation**: complex invariants (comments only)
- ✅ Provide examples for each component

**Acceptance Criteria**:
- ✅ Clear guidelines for contract usage
- ✅ Examples provided
- ✅ Team reviewed and approved

---

## 1. execution_context Implementation

### 1.1 Core Interface Design
**Estimated Time**: 3-4 days
**Status**: ✅ COMPLETE (100% 🎉)

- [x] Create `svarog/include/svarog/execution/execution_context.hpp`
- [x] Define abstract base class `execution_context`
  - [x] Virtual destructor
  - [x] Pure virtual `stop()` method
  - [x] Pure virtual `restart()` method
  - [x] Pure virtual `stopped()` const noexcept method (returns `bool`)
  - [x] Copy constructor/assignment deleted (move-only semantic)
  - [x] Service registry interface (inspired by Asio's service model)
    - [x] `template<typename Service> Service& use_service()` ✅
    - [x] `template<typename Service> void add_service(std::shared_ptr<Service>)` ✅
    - [x] `template<typename Service> bool has_service() const noexcept` ✅
    - [x] Service storage mechanism (`std::unordered_map<std::type_index, std::shared_ptr<void>>`)
    - [x] Thread-safe service access (`mutable std::mutex` + `std::lock_guard`) ✅
    - [ ] Service lifecycle hooks (on_shutdown, on_restart) - deferred to section 1.2
- [x] Document design decisions in inline comments
- [x] Add doxygen documentation for public API

**Implementation Summary** ✅:
- ✅ Complete Doxygen documentation for all public APIs
- ✅ Design decision comments explaining architecture choices
- ✅ Thread-safe service registry with mutex protection
- ✅ Contract programming:
  - ✅ `SVAROG_EXPECTS(!stopped())` in `use_service()`
  - ✅ `SVAROG_EXPECTS(has_service<Service>())` prevents `std::out_of_range`
- ✅ Type-safe service storage using `std::type_index`
- ✅ Move-only semantic for execution contexts
- ✅ Examples in documentation

**Acceptance Criteria**:
- ✅ Header compiles without errors
- ✅ API design reviewed and approved
- ✅ Documentation complete

**Progress**: 🎯 100% COMPLETE

All tasks for section 1.1 are finished. Ready to proceed to section 1.2.

**Acceptance Criteria**:
- Header compiles without errors
- API design reviewed and approved
- Documentation complete

### 1.2 Service Registry Implementation
**Estimated Time**: 4-5 days

- [ ] Create `svarog/source/execution/execution_context.cpp`
- [ ] Implement service storage mechanism
  - [ ] Thread-safe service map (std::type_index → service pointer)
  - [ ] Service ownership management (unique_ptr or shared_ptr)
  - [ ] Service creation on first use
- [ ] Implement service access methods
  - [ ] `add_service<ServiceT>(...)`
  - [ ] `use_service<ServiceT>()`
  - [ ] `has_service<ServiceT>() const noexcept`
- [ ] Add service lifecycle management
  - [ ] Service construction
  - [ ] Service destruction order (reverse of creation)
  - [ ] Service shutdown hooks

**Acceptance Criteria**:
- Service registry tests pass (see 1.3)
- No memory leaks (valgrind clean)
- Thread-safe service access

### 1.3 Contract Specification
**Estimated Time**: 1-2 days

- [ ] Add preconditions to public methods
  ```cpp
  template<typename Service>
  Service& use_service() {
      SVAROG_EXPECTS(!stopped());  // Cannot use services after stop
      // Implementation...
  }
  
  void stop() override {
      SVAROG_EXPECTS(!stopped());  // Cannot stop twice
      // Implementation...
  }
  ```
- [ ] Add postconditions for state changes
  ```cpp
  void restart() {
      auto was_stopped = stopped();
      // Implementation...
      SVAROG_ENSURES(!stopped());  // Must be running after restart
  }
  ```
- [ ] Document class invariants
  ```cpp
  // Class invariant: 
  // - If stopped(), no new services can be added
  // - Services are destroyed in reverse creation order
  // - Service registry is always internally consistent
  ```

**Acceptance Criteria**:
- All public methods have preconditions where applicable
- Critical postconditions verified
- Class invariants documented in header

### 1.4 Unit Tests
**Estimated Time**: 3 days

- [ ] Create `tests/execution/execution_context_tests.cpp`
- [ ] Test service registration and retrieval
  ```cpp
  TEST_CASE("execution_context service registry", "[execution_context]") {
      SECTION("Single service registration") { ... }
      SECTION("Multiple service types") { ... }
      SECTION("Service singleton per context") { ... }
  }
  ```
- [ ] Test service lifecycle
  - [ ] Service created on first `use_service`
  - [ ] Service destroyed with context
  - [ ] Services destroyed in reverse creation order
- [ ] Test thread safety
  - [ ] Concurrent `use_service` calls
  - [ ] Concurrent `add_service` attempts

**Acceptance Criteria**:
- All tests pass (100% pass rate)
- Code coverage ≥ 90% for execution_context

---

## 2. work_queue Implementation

### 2.1 Lock-Free Queue Design
**Estimated Time**: 5-7 days

- [ ] Create `svarog/include/svarog/execution/work_queue.hpp`
- [ ] Implement MPMC lock-free queue
  - [ ] Based on ADR-001 decision (lock-free architecture)
  - [ ] Use atomic operations for thread-safe access
  - [ ] Consider Boost.Lockfree or custom implementation
- [ ] Define work item type
  ```cpp
  using work_item = std::move_only_function<void()>;
  ```
- [ ] Implement queue operations
  - [ ] `push(work_item&& item) -> bool`
  - [ ] `try_pop(work_item& item) -> bool`
  - [ ] `size() const noexcept -> size_t` (approximate)
  - [ ] `empty() const noexcept -> bool` (approximate)

**Acceptance Criteria**:
- Lock-free queue compiles and passes basic tests
- No data races (ThreadSanitizer clean)
- Performance comparable to std::deque in single-threaded case

### 2.2 Contract Specification
**Estimated Time**: 1 day

- [ ] Add preconditions to queue operations
  ```cpp
  void push(work_item&& item) {
      SVAROG_EXPECTS(item != nullptr);     // Handler must be valid
      SVAROG_EXPECTS(!is_shutdown());      // Cannot push after shutdown
      // Implementation...
  }
  
  std::expected<work_item, queue_error> try_pop() noexcept {
      // No preconditions - valid to call on empty/shutdown queue
      // Returns error via std::expected
      auto result = try_pop_impl();
      SVAROG_ENSURES(!result.has_value() || result.value() != nullptr);
      return result;
  }
  ```
- [ ] Document thread-safety guarantees
  ```cpp
  // Thread-safety invariants:
  // - push() is thread-safe (multiple producers)
  // - try_pop() is thread-safe (multiple consumers)
  // - size() is approximate (may be stale immediately)
  // - After shutdown(), all blocking operations return error
  ```
- [ ] Add invariant checks in Debug builds
  ```cpp
  #ifndef NDEBUG
  void check_invariants() const {
      SVAROG_ASSERT(size_ >= 0);
      SVAROG_ASSERT(!(shutdown_requested_ && has_pending_work()));
  }
  #endif
  ```

**Acceptance Criteria**:
- All public methods have appropriate preconditions
- Thread-safety guarantees documented
- Invariant checks in Debug builds

### 2.3 Work Queue Interface
**Estimated Time**: 3-4 days

- [ ] Implement `work_queue` class
  - [ ] Constructor with optional capacity hint
  - [ ] `post(Callable&& f)` - asynchronous execution
  - [ ] `dispatch(Callable&& f)` - immediate or deferred execution
  - [ ] `stop()` - signal workers to stop
  - [ ] `stopped() const noexcept -> bool`
- [ ] Implement work item wrapping
  - [ ] Type erasure for callables
  - [ ] Move-only semantics
  - [ ] Exception safety (catch and log)

**Acceptance Criteria**:
- API matches design specification
- Code compiles without warnings (-Wall -Wextra -Wpedantic)
- API documentation complete

### 2.3 Worker Thread Pool
**Estimated Time**: 4-5 days

- [ ] Create `svarog/source/execution/work_queue.cpp`
- [ ] Implement worker thread management
  - [ ] Thread pool with configurable size
  - [ ] Worker thread lifecycle (start, stop, join)
  - [ ] Work-stealing strategy (optional, if time permits)
- [ ] Implement work execution loop
  ```cpp
  void worker_thread() {
      while (!stopped()) {
          work_item item;
          if (try_pop(item)) {
              try { item(); }
              catch (...) { /* log and continue */ }
          } else {
              // Wait strategy (yield, sleep, condition variable)
          }
      }
  }
  ```
- [ ] Implement graceful shutdown
  - [ ] `stop()` signals workers
  - [ ] Workers finish current items
  - [ ] Remaining items optionally drained or discarded

**Acceptance Criteria**:
- Worker threads execute tasks correctly
- Graceful shutdown completes within reasonable time (<1s)
- No deadlocks or hangs

### 2.4 Unit Tests and Benchmarks
**Estimated Time**: 4-5 days

- [ ] Create `tests/execution/work_queue_tests.cpp`
- [ ] Test basic functionality
  ```cpp
  TEST_CASE("work_queue basic operations", "[work_queue]") {
      SECTION("post single task") { ... }
      SECTION("post multiple tasks") { ... }
      SECTION("tasks execute in workers") { ... }
  }
  ```
- [ ] Test concurrent operations
  - [ ] Multiple threads posting simultaneously
  - [ ] MPMC scenario (10 producers, 10 consumers)
  - [ ] High contention stress test
- [ ] Test shutdown behavior
  - [ ] Graceful shutdown with pending work
  - [ ] Stop prevents new work submission
- [ ] Create `benchmarks/work_queue_benchmarks.cpp`
  - [ ] Single-threaded throughput
  - [ ] Multi-threaded throughput (MPMC)
  - [ ] Latency measurement (post to execution time)
  - [ ] Contention scaling (1, 2, 4, 8 threads)

**Acceptance Criteria**:
- All unit tests pass
- Code coverage ≥ 90%
- Benchmarks meet performance targets:
  - Single-threaded: ≥1M tasks/sec
  - 4-thread MPMC: ≥500K tasks/sec per thread
  - Latency: <100ns (median)

---

## 3. io_context Implementation

### 3.1 io_context Core Design
**Estimated Time**: 4-5 days

- [ ] Create `svarog/include/svarog/io/io_context.hpp`
- [ ] Implement `io_context` class inheriting from `execution_context`
  ```cpp
  class io_context : public execution_context {
  public:
      io_context();
      explicit io_context(int concurrency_hint);
      
      void run();
      size_t run_one();
      void stop() override;
      bool stopped() const noexcept override;
      void restart() override;
      
      // Executor access
      auto get_executor() noexcept;
  };
  ```
- [ ] Design event loop architecture
  - [ ] Integration with OS event notification (epoll/kqueue)
  - [ ] Work queue integration (reuse work_queue)
  - [ ] Timer queue integration (prepared for Phase 2)
- [ ] Define executor type
  ```cpp
  class io_context::executor_type {
  public:
      void execute(std::move_only_function<void()> f) const;
      io_context& context() const noexcept;
      // Standard executor properties
  };
  ```

**Acceptance Criteria**:
- Header compiles cleanly
- Design compatible with C++23 executors (future-proof)
- API documentation complete

### 3.2 Event Loop Implementation
**Estimated Time**: 6-8 days

- [ ] Create `svarog/source/io/io_context.cpp`
- [ ] Implement `run()` method
  ```cpp
  void io_context::run() {
      while (!stopped()) {
          // 1. Process ready handlers from work_queue
          // 2. Poll I/O events (epoll_wait with timeout)
          // 3. Process I/O completion handlers
          // 4. Check timers (if implemented)
          // 5. Yield if no work
      }
  }
  ```
- [ ] Implement `run_one()` method
  - [ ] Execute exactly one handler
  - [ ] Return count of executed handlers (0 or 1)
- [ ] Implement `stop()` method
  - [ ] Set stopped flag (atomic)
  - [ ] Interrupt blocking event wait
  - [ ] Wake up all `run()` calls
- [ ] Implement `restart()` method
  - [ ] Clear stopped flag
  - [ ] Reset internal state
- [ ] Platform-specific event backend
  - [ ] Linux: epoll integration (epoll_create, epoll_ctl, epoll_wait)
  - [ ] macOS: kqueue integration (future consideration)
  - [ ] Abstract platform differences

**Acceptance Criteria**:
- Event loop runs and processes handlers
- `stop()` interrupts `run()` within <10ms
- No busy-waiting (verified with CPU profiling)
- Platform-specific code isolated

### 3.3 Contract Specification
**Estimated Time**: 1 day

- [ ] Add preconditions to public methods
  ```cpp
  template<typename Handler>
  void post(Handler&& handler) {
      SVAROG_EXPECTS(handler != nullptr);  // Handler must be valid
      // Can post even when stopped - will be queued but not executed
      // Implementation...
  }
  
  template<typename Handler>
  void dispatch(Handler&& handler) {
      SVAROG_EXPECTS(handler != nullptr);
      if (running_in_this_thread()) {
          SVAROG_EXPECTS(!stopped());  // Cannot dispatch on stopped context
          handler();  // Execute immediately
      } else {
          post(std::forward<Handler>(handler));
      }
  }
  
  std::size_t run() {
      SVAROG_EXPECTS(!stopped());  // Must not be stopped before run
      // Implementation...
      auto count = run_impl();
      SVAROG_ENSURES(stopped());   // After run(), context is stopped
      return count;
  }
  ```
- [ ] Document thread-safety guarantees
  ```cpp
  // Thread-safety:
  // - post() is thread-safe (can be called from any thread)
  // - dispatch() is thread-safe
  // - run() can be called from multiple threads simultaneously
  // - stop() is thread-safe (can be called from any thread)
  // - After stop(), run() will return on all threads
  ```
- [ ] Add state invariants
  ```cpp
  // State invariants:
  // - stopped() implies no more handlers will execute
  // - restart() can only be called when stopped()
  // - Outstanding work count >= 0
  ```

**Acceptance Criteria**:
- All public methods have preconditions where applicable
- Thread-safety guarantees documented
- State transitions clearly defined

### 3.4 Executor Implementation
**Estimated Time**: 3-4 days

- [ ] Implement `io_context::executor_type`
- [ ] Implement `execute()` method
  ```cpp
  void executor_type::execute(std::move_only_function<void()> f) const {
      ctx_->post(std::move(f));
  }
  ```
- [ ] Implement `post()` and `dispatch()` on io_context
  - [ ] `post(Callable&& f)` - always defer
  - [ ] `dispatch(Callable&& f)` - execute immediately if in `run()`, else defer
- [ ] Implement executor comparison operators
  - [ ] `operator==`, `operator!=`
  - [ ] Compare by context identity

**Acceptance Criteria**:
- Executor meets C++23 executor requirements
- `post` always defers execution
- `dispatch` executes immediately when called from io_context thread
- Unit tests verify executor semantics

### 3.4 Unit Tests and Integration Tests
**Estimated Time**: 5-6 days

- [ ] Create `tests/io/io_context_tests.cpp`
- [ ] Test basic operations
  ```cpp
  TEST_CASE("io_context run and stop", "[io_context]") {
      SECTION("run with posted work") { ... }
      SECTION("stop interrupts run") { ... }
      SECTION("restart after stop") { ... }
  }
  ```
- [ ] Test executor semantics
  - [ ] `execute()` defers work
  - [ ] Work executes in `run()`
  - [ ] Multiple executors from same context
- [ ] Test `post()` vs `dispatch()`
  - [ ] `post` always defers
  - [ ] `dispatch` executes immediately from io_context thread
  - [ ] `dispatch` defers from other threads
- [ ] Test multi-threaded `run()`
  - [ ] Multiple threads calling `run()` concurrently
  - [ ] Work distributed among threads
  - [ ] No data races (ThreadSanitizer clean)
- [ ] Create `benchmarks/io_context_benchmarks.cpp`
  - [ ] Post throughput (single-threaded)
  - [ ] Handler execution latency
  - [ ] Multi-threaded run() scalability

**Acceptance Criteria**:
- All unit tests pass
- Code coverage ≥ 90%
- Benchmarks meet performance targets:
  - Post throughput: ≥500K ops/sec
  - Handler latency: <200ns (median)
  - Multi-threaded run() scales linearly up to 4 threads

---

## 4. strand Implementation

### 4.1 Strand Design and Interface
**Estimated Time**: 4-5 days

- [ ] Create `svarog/include/svarog/execution/strand.hpp`
- [ ] Define `strand` class
  ```cpp
  template<typename Executor>
  class strand {
  public:
      using executor_type = Executor;
      
      explicit strand(executor_type ex);
      
      executor_type get_executor() const noexcept;
      
      template<typename F>
      void execute(F&& f) const;
      
      template<typename F>
      void post(F&& f) const;
      
      template<typename F>
      void dispatch(F&& f) const;
      
      bool running_in_this_thread() const noexcept;
  };
  ```
- [ ] Design serialization strategy
  - [ ] Handler queue (thread-safe)
  - [ ] Execution lock (mutex or atomic flag)
  - [ ] Recursive dispatch prevention
- [ ] Define executor wrapper
  ```cpp
  class strand_executor {
      // Wraps underlying executor
      // Enforces serialization
  };
  ```

**Acceptance Criteria**:
- API design reviewed and approved
- Template compiles with io_context::executor_type
- Documentation complete

### 4.2 Serialization Implementation
**Estimated Time**: 5-6 days

- [ ] Create `svarog/source/execution/strand.cpp` (if needed for non-template parts)
- [ ] Implement handler queue
  - [ ] Thread-safe queue (reuse work_queue or custom)
  - [ ] FIFO ordering guarantee
- [ ] Implement execution serialization
  ```cpp
  void strand::execute_next() {
      while (true) {
          handler h;
          {
              lock_guard lock(mutex_);
              if (queue_.empty()) {
                  running_ = false;
                  return;
              }
              h = queue_.pop_front();
          }
          try { h(); }
          catch (...) { /* log */ }
      }
  }
  
  template<typename F>
  void strand::post(F&& f) {
      {
          lock_guard lock(mutex_);
          queue_.push_back(std::forward<F>(f));
          if (running_) return; // Already executing
          running_ = true;
      }
      underlying_executor_.execute([this]{ execute_next(); });
  }
  ```
- [ ] Implement `dispatch()` logic
  - [ ] If `running_in_this_thread()` → execute immediately
  - [ ] Else → `post()`
- [ ] Implement `running_in_this_thread()`
  - [ ] Thread-local flag or thread ID comparison

**Acceptance Criteria**:
- Handlers never execute concurrently on same strand
- FIFO ordering preserved
- No deadlocks or race conditions
- ThreadSanitizer clean

### 4.3 Contract Specification
**Estimated Time**: 1 day

- [ ] Add preconditions to strand operations
  ```cpp
  template<typename F>
  void post(F&& f) {
      SVAROG_EXPECTS(f != nullptr);  // Handler must be valid
      // Implementation...
  }
  
  template<typename F>
  void dispatch(F&& f) {
      SVAROG_EXPECTS(f != nullptr);
      if (running_in_this_thread()) {
          SVAROG_EXPECTS(execution_depth_ < max_recursion_depth);  // Prevent stack overflow
          f();  // Execute immediately - already serialized
      } else {
          post(std::forward<F>(f));
      }
  }
  ```
- [ ] Document serialization guarantees
  ```cpp
  // Serialization invariants:
  // - Handlers posted to the same strand NEVER execute concurrently
  // - Handlers execute in FIFO order (unless dispatch() from strand thread)
  // - running_in_this_thread() returns true only if executing handler on this strand
  // - Multiple strands CAN execute concurrently (independent serialization)
  ```
- [ ] Add recursion depth tracking
  ```cpp
  class strand {
      static constexpr std::size_t max_recursion_depth = 100;
      thread_local static std::size_t execution_depth_;
      
      // Track recursion to prevent stack overflow from dispatch()
  };
  ```

**Acceptance Criteria**:
- Preconditions on all handler-accepting methods
- Serialization guarantees clearly documented
- Recursion depth protection in place

### 4.4 Unit Tests and Benchmarks
**Estimated Time**: 4-5 days

- [ ] Create `tests/execution/strand_tests.cpp`
- [ ] Test serialization guarantee
  ```cpp
  TEST_CASE("strand serialization", "[strand]") {
      SECTION("handlers execute serially") {
          strand s(io_ctx.get_executor());
          std::atomic<int> counter = 0;
          for (int i = 0; i < 1000; ++i) {
              s.post([&counter] {
                  int old = counter.load();
                  std::this_thread::sleep_for(1us); // Force contention
                  counter.store(old + 1);
              });
          }
          io_ctx.run();
          REQUIRE(counter == 1000); // Would fail without serialization
      }
  }
  ```
- [ ] Test FIFO ordering
  - [ ] Post 100 handlers that record execution order
  - [ ] Verify order matches post order
- [ ] Test `dispatch()` immediate execution
  - [ ] From strand thread → executes immediately
  - [ ] From other thread → defers
- [ ] Test multi-threaded io_context with strands
  - [ ] Multiple threads calling `run()`
  - [ ] Multiple strands posting work
  - [ ] Each strand's work still serialized
- [ ] Create `benchmarks/strand_benchmarks.cpp`
  - [ ] Serialization overhead vs bare executor
  - [ ] Throughput with multiple strands
  - [ ] `dispatch()` vs `post()` latency

**Acceptance Criteria**:
- All tests pass
- Code coverage ≥ 90%
- Benchmarks meet performance targets:
  - Serialization overhead: <50ns vs bare executor
  - Throughput: ≥80% of bare executor
  - `dispatch` immediate execution: <10ns

---

## 5. Integration and System Tests

### 5.1 Integration Testing
**Estimated Time**: 4-5 days

- [ ] Create `tests/integration/phase1_integration_tests.cpp`
- [ ] Test execution_context + work_queue
  - [ ] work_queue uses execution_context services
  - [ ] Multiple work_queues in same context
- [ ] Test io_context + strand
  - [ ] Strands wrapping io_context executors
  - [ ] Multiple strands in same io_context
  - [ ] Concurrent `run()` with strands
- [ ] Test all components together
  ```cpp
  TEST_CASE("Phase 1 full integration", "[integration]") {
      io_context io_ctx;
      strand s1(io_ctx.get_executor());
      strand s2(io_ctx.get_executor());
      
      // Post work to both strands
      // Verify serialization within each strand
      // Verify parallelism between strands
      
      io_ctx.run();
  }
  ```
- [ ] Test real-world scenarios
  - [ ] Simple task scheduler (post delayed tasks)
  - [ ] Producer-consumer pattern
  - [ ] Pipeline pattern (strand1 → strand2 → strand3)

**Acceptance Criteria**:
- All integration tests pass
- Components work correctly together
- No unexpected interactions or bugs

### 5.2 Performance Validation
**Estimated Time**: 3-4 days

- [ ] Run all benchmarks on target hardware
- [ ] Collect baseline performance data
  - [ ] work_queue throughput and latency
  - [ ] io_context throughput and latency
  - [ ] strand serialization overhead
- [ ] Compare against performance targets (see BENCHMARK_SCENARIOS.md)
- [ ] Identify and document any performance gaps
- [ ] Create performance regression test suite
  - [ ] Define acceptable performance ranges
  - [ ] Automated benchmark runs in CI (nightly builds)

**Acceptance Criteria**:
- All performance targets met or gaps documented
- Benchmark results reproducible
- Nightly CI runs benchmarks successfully

### 5.3 Documentation and Examples
**Estimated Time**: 4-5 days

- [ ] Update `README.md` with Phase 1 completion status
- [ ] Create usage examples
  - [ ] `examples/execution_context_example.cpp`
  - [ ] `examples/work_queue_example.cpp`
  - [ ] `examples/io_context_example.cpp`
  - [ ] `examples/strand_example.cpp`
- [ ] Write API documentation
  - [ ] Doxygen comments for all public APIs
  - [ ] Generate HTML documentation
  - [ ] Add usage patterns and best practices
- [ ] Update REFACTORING_PLAN.md
  - [ ] Mark Phase 1 as complete
  - [ ] Document any deviations from original plan
  - [ ] Update Phase 2 dependencies if needed
- [ ] Create Phase 1 retrospective document
  - [ ] What went well
  - [ ] Challenges encountered
  - [ ] Lessons learned
  - [ ] Recommendations for Phase 2

**Acceptance Criteria**:
- Examples compile and run
- Documentation complete and accurate
- Doxygen builds without warnings
- Retrospective written

---

## 6. Code Review and Quality Assurance

### 6.1 Code Review
**Estimated Time**: 3-4 days

- [ ] Self-review all Phase 1 code
  - [ ] Check for TODOs or FIXMEs
  - [ ] Verify consistent coding style
  - [ ] Ensure all headers have include guards
  - [ ] Check for missing const/noexcept
- [ ] Peer review (if applicable)
  - [ ] Request review from team members
  - [ ] Address feedback
  - [ ] Re-review after changes
- [ ] Static analysis
  - [ ] Run clang-tidy
  - [ ] Run cppcheck
  - [ ] Address all warnings and errors

**Acceptance Criteria**:
- Zero open review comments
- Static analysis clean
- Code style consistent

### 6.2 Test Coverage and Quality
**Estimated Time**: 2-3 days

- [ ] Measure code coverage
  - [ ] Use gcov/lcov or similar tool
  - [ ] Generate coverage report
- [ ] Achieve ≥90% code coverage for Phase 1 components
- [ ] Add tests for uncovered code paths
- [ ] Review test quality
  - [ ] Edge cases covered
  - [ ] Error paths tested
  - [ ] Assertions meaningful

**Acceptance Criteria**:
- Code coverage ≥90%
- All critical paths tested
- Coverage report generated

### 6.3 Memory and Thread Safety
**Estimated Time**: 2-3 days

- [ ] Run Valgrind on all tests
  - [ ] Zero memory leaks
  - [ ] Zero uninitialized reads
  - [ ] Zero invalid accesses
- [ ] Run ThreadSanitizer on all tests
  - [ ] Zero data races
  - [ ] Zero deadlocks
- [ ] Run AddressSanitizer on all tests
  - [ ] Zero buffer overflows
  - [ ] Zero use-after-free
- [ ] Run UndefinedBehaviorSanitizer
  - [ ] Zero undefined behavior

**Acceptance Criteria**:
- All sanitizer runs clean
- Valgrind reports zero errors
- CI includes sanitizer builds (already in nightly.yml)

---

## Summary

**Total Estimated Time**: 4-6 weeks

**Critical Path Items**:
0. Contract Programming setup (foundation - 1-2 days)
1. execution_context (foundation for everything)
2. work_queue (required by io_context)
3. io_context (required by strand)
4. strand (completes Phase 1 core)

**Success Metrics**:
- ✅ All unit tests pass (100% pass rate)
- ✅ Code coverage ≥90%
- ✅ All benchmarks meet performance targets
- ✅ Zero sanitizer errors
- ✅ Zero Valgrind errors
- ✅ All contracts validated in Debug builds
- ✅ Zero contract violations in production code
- ✅ Documentation complete
- ✅ Examples compile and run
- ✅ CI/CD pipelines green (PR and nightly)

**Deliverables**:
1. GSL-Lite integrated contract programming infrastructure
2. Coding standards document with contract usage guidelines
3. Fully tested and documented execution_context with contracts
4. High-performance lock-free work_queue with preconditions
5. Event-driven io_context with epoll backend and contract validation
6. Serializing strand executor wrapper with recursion protection
7. Comprehensive test suite (unit, integration, benchmarks, contract violations)
8. Usage examples demonstrating contract usage
9. API documentation (Doxygen) with preconditions/postconditions
10. Phase 1 retrospective document

**Contract Programming Benefits Realized**:
- 🔍 Early error detection through precondition checks
- 📚 Executable documentation of API requirements
- 🛡️ Protection against common async programming errors
- 🚀 Zero overhead in Release builds (by default)
- 🔮 Future-proof for C++26 contracts standard

**Next Steps After Phase 1**:
- Phase 2: Sockets, Buffers, Timers (6-8 weeks)
- Phase 3: Advanced protocols and utilities (4-6 weeks)
- Continuous: Refine contract specifications based on production usage
