# svarogIO Coding Standards

## Overview

This document defines coding standards and best practices for the svarogIO project. Following these standards ensures consistency, maintainability, and high code quality across the codebase.

**Version:** 1.0  
**Date:** 2025-11-10  
**Language Standard:** C++23

---

## 1. Contract Programming

### 1.1 Philosophy

**Contracts define the expectations between callers and callees.** Use GSL `Expects`/`Ensures` macros (via `SVAROG_EXPECTS`/`SVAROG_ENSURES`) to document and enforce these contracts.

### 1.2 When to Use Contracts vs std::expected

#### Use SVAROG_EXPECTS for:
- **Programming Errors** - Bugs that should never happen in correct code
- **API Misuse** - Caller violated documented requirements
- **Resource Ownership Violations** - Accessing invalid/null resources

```cpp
// ✅ GOOD: Programming error - handler must never be null
void io_context::post(std::move_only_function<void()> handler) {
    SVAROG_EXPECTS(handler != nullptr);
    impl_->push(std::move(handler));
}

// ✅ GOOD: API misuse - cannot call run() when stopped
std::size_t io_context::run() {
    SVAROG_EXPECTS(!stopped());
    return run_impl();
}
```

#### Use std::expected for:
- **Expected Runtime Errors** - Errors that can legitimately occur
- **Business Logic Failures** - Invalid data, network errors, etc.
- **Recoverable Errors** - Caller can handle the error

```cpp
// ✅ GOOD: Expected error - queue can legitimately be empty
std::expected<work_item, queue_error> work_queue::try_pop() noexcept {
    std::lock_guard lock(mutex_);
    if (queue_.empty()) {
        return std::unexpected(queue_error::empty);
    }
    return queue_.pop_front();
}

// ✅ GOOD: Network error is expected and recoverable
std::expected<size_t, std::error_code> socket::receive(buffer& buf) {
    auto bytes = ::recv(fd_, buf.data(), buf.size(), 0);
    if (bytes < 0) {
        return std::unexpected(std::error_code{errno, std::system_category()});
    }
    return static_cast<size_t>(bytes);
}
```

#### ❌ Anti-patterns

```cpp
// ❌ BAD: Using std::expected for programming error
std::expected<void, Error> io_context::post(Handler&& h) {
    if (!h) {
        return std::unexpected(Error::NullHandler);  // Should be SVAROG_EXPECTS!
    }
    // ...
}

// ❌ BAD: Using SVAROG_EXPECTS for expected error
int parse_int(const std::string& s) {
    SVAROG_EXPECTS(!s.empty());  // User input can be empty! Use std::expected
    return std::stoi(s);
}
```

### 1.3 Contract Categories

#### 🔴 Critical Contracts (Always Enabled)

For invariants that **must never be violated**, even in Release builds.

```cpp
void critical_operation(resource* res) {
    // Always check critical invariants
    #ifdef SVAROG_ENABLE_CONTRACTS_IN_RELEASE
    SVAROG_EXPECTS(res != nullptr);
    SVAROG_EXPECTS(res->is_valid());
    #else
    assert(res != nullptr && "Critical: resource must not be null");
    #endif
    
    // ...
}
```

**Use for:**
- Memory safety (null pointer dereference)
- Resource ownership violations
- Security-critical checks

#### 🟡 Debug-Only Contracts (Default)

For checks that help during development but have performance cost.

```cpp
template<typename T>
void work_queue::push(T&& item) {
    SVAROG_EXPECTS(!is_shutdown());  // Removed in Release builds
    
    // Hot path - contract overhead matters
    queue_.push_back(std::forward<T>(item));
}
```

**Use for:**
- Range checks in hot paths
- State validation in performance-critical code
- Complex invariant checks

#### 🟢 Documentation Contracts

For complex invariants better expressed as comments.

```cpp
class strand {
    // INVARIANTS:
    // - Handlers on same strand NEVER execute concurrently
    // - Handlers execute in FIFO order (except dispatch())
    // - running_in_this_thread() == true only during handler execution
    
    // Too complex to check efficiently at runtime
};
```

### 1.4 Preconditions (SVAROG_EXPECTS)

**Preconditions** define what must be true **before** a function executes.

```cpp
void strand::dispatch(Handler&& handler) {
    SVAROG_EXPECTS(handler != nullptr);  // Handler must be valid
    SVAROG_EXPECTS(execution_depth_ < max_recursion_depth);  // Prevent stack overflow
    
    // ...
}
```

**Guidelines:**
- ✅ Place at the **beginning** of the function
- ✅ Check parameters and object state
- ✅ Document in function comment
- ❌ Don't use for checking return values of internal calls

### 1.5 Postconditions (SVAROG_ENSURES)

**Postconditions** define what must be true **after** a function executes.

```cpp
std::size_t io_context::run() {
    SVAROG_EXPECTS(!stopped());  // Precondition
    
    auto count = run_impl();
    
    SVAROG_ENSURES(stopped());    // Postcondition: always stopped after run
    SVAROG_ENSURES(count >= 0);   // Result is valid
    return count;
}
```

**Guidelines:**
- ✅ Place just **before** return statements
- ✅ Verify results and state changes
- ✅ Use for critical guarantees
- ❌ Avoid expensive checks in hot paths

### 1.6 Invariants

**Invariants** are conditions that must **always** be true for an object.

```cpp
class work_queue {
public:
    void push(work_item item) {
        check_invariants();  // Check before modification
        
        // Modify state
        queue_.push_back(std::move(item));
        
        check_invariants();  // Check after modification
    }
    
private:
    void check_invariants() const {
        #ifndef NDEBUG
        SVAROG_ASSERT(size_ == queue_.size());
        SVAROG_ASSERT(!shutdown_ || queue_.empty());
        SVAROG_ASSERT(size_ >= 0);
        #endif
    }
    
    // INVARIANTS (documented):
    // - size_ always equals queue_.size()
    // - After shutdown(), queue is empty
    // - size_ is never negative
};
```

---

## 2. Error Handling

### 2.1 Error Handling Strategy

| Error Type | Mechanism | Example |
|------------|-----------|---------|
| Programming errors | `SVAROG_EXPECTS` | Null pointer, invalid state |
| Expected runtime errors | `std::expected` | Network errors, file not found |
| Exceptional errors | Exceptions | Resource exhaustion, OOM |

### 2.2 std::expected Usage

```cpp
// ✅ GOOD: Clear error type, noexcept
std::expected<T, queue_error> try_pop() noexcept {
    if (queue_.empty()) {
        return std::unexpected(queue_error::empty);
    }
    return queue_.front();
}

// ✅ GOOD: Monadic error handling
auto result = queue.try_pop()
    .and_then([](auto item) { return process(item); })
    .or_else([](auto err) { log_error(err); });
```

### 2.3 Exception Policy

**Use exceptions only for:**
- Resource exhaustion (std::bad_alloc)
- Logic errors that can't continue (std::logic_error)
- External library errors (when unavoidable)

**Don't use exceptions for:**
- Control flow
- Expected errors (use std::expected)
- Performance-critical paths

```cpp
// ✅ GOOD: Exception for unexpected resource issue
void io_context_impl::init_epoll() {
    epoll_fd_ = ::epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd_ < 0) {
        throw std::runtime_error("Failed to create epoll instance");
    }
}

// ❌ BAD: Exception for expected condition
T work_queue::pop() {
    if (queue_.empty()) {
        throw queue_empty_error();  // Use std::expected instead!
    }
    return queue_.front();
}
```

---

## 3. Thread Safety

### 3.1 Documentation

**Always document thread-safety guarantees** for public APIs.

```cpp
class work_queue {
public:
    // Thread-safe: can be called from multiple threads simultaneously
    void push(work_item item);
    
    // Thread-safe: can be called from multiple threads simultaneously
    std::expected<work_item, queue_error> try_pop() noexcept;
    
    // NOT thread-safe: must be called from single thread
    void shutdown();
    
private:
    // Protected by mutex_
    std::deque<work_item> queue_;
    mutable std::mutex mutex_;
};
```

### 3.2 Strand Usage

**Prefer strand over manual locking** for asynchronous code.

```cpp
// ✅ GOOD: Strand serializes access - no mutex needed
class Connection {
public:
    explicit Connection(io_context& ctx) : strand_(ctx) {}
    
    void async_read() {
        strand_.post([this] {
            // No mutex needed - strand guarantees serialization
            buffer_ += read_data();
        });
    }
    
    void async_write() {
        strand_.post([this] {
            // No mutex needed - strand guarantees serialization
            write_data(buffer_);
        });
    }
    
private:
    strand<io_context> strand_;
    std::string buffer_;  // No mutex needed!
};

// ❌ BAD: Manual mutex in async code
class Connection {
    void async_read() {
        io_ctx_.post([this] {
            std::lock_guard lock(mutex_);  // Unnecessary with strand!
            buffer_ += read_data();
        });
    }
    
    std::mutex mutex_;
    std::string buffer_;
};
```

---

## 4. RAII and Resource Management

### 4.1 Smart Pointers

**Always use smart pointers for ownership.**

```cpp
// ✅ GOOD: Clear ownership with unique_ptr
class io_context {
    std::unique_ptr<impl> impl_;
public:
    io_context() : impl_(std::make_unique<impl>()) {}
};

// ❌ BAD: Manual memory management
class io_context {
    impl* impl_;
public:
    io_context() : impl_(new impl()) {}
    ~io_context() { delete impl_; }
};
```

### 4.2 work_guard Pattern

**Use work_guard to manage io_context lifetime.**

```cpp
// ✅ GOOD: work_guard prevents premature stop
{
    io_context io_ctx;
    auto guard = make_work_guard(io_ctx);
    
    // Post async work
    io_ctx.post([]{ /* ... */ });
    
    // Run won't exit while guard alive
    std::thread t([&]{ io_ctx.run(); });
    
    // ... do work ...
    
    guard.reset();  // Explicit release
    t.join();
}
```

---

## 5. C++23 Features

### 5.1 std::move_only_function

**Use for handlers that capture move-only types.**

```cpp
// ✅ GOOD: Supports move-only captures
using work_item = std::move_only_function<void()>;

io_ctx.post([socket = std::move(unique_socket)] {
    socket->async_read(/* ... */);
});

// ❌ BAD: std::function requires copyable
using work_item = std::function<void()>;  // Can't capture unique_ptr!
```

### 5.2 Deducing this

**Use for reducing overload count.**

```cpp
// ✅ GOOD: Single method with deducing this
template<typename Self>
void push(this Self&& self, auto&& item) {
    std::lock_guard lock(self.mutex_);
    self.queue_.push_back(std::forward<decltype(item)>(item));
}

// ❌ Verbose: Two overloads
void push(const T& item);
void push(T&& item);
```

---

## 6. Code Style

### 6.1 Naming Conventions

```cpp
namespace svarog::execution {  // snake_case

class io_context {             // snake_case
public:
    void run();                // snake_case
    bool stopped() const;      // snake_case
    
private:
    std::size_t count_;        // trailing underscore for members
    static constexpr int max_threads = 100;  // snake_case
};

} // namespace svarog::execution
```

### 6.2 File Organization

```
svarog/
├── include/svarog/
│   ├── execution/
│   │   ├── execution_context.hpp     # Public API
│   │   └── io_context.hpp
│   └── core/
│       └── contracts.hpp              # Contract macros
│
└── source/svarog/
    └── execution/
        ├── io_context.cpp             # Implementation
        └── io_context_impl.hpp        # Pimpl (private header)
```

---

## 7. Testing

### 7.1 Test Organization

```cpp
TEST_CASE("Component: what it tests", "[tag]") {
    SECTION("specific scenario") {
        // Arrange
        io_context ctx;
        
        // Act
        ctx.post([]{ /* ... */ });
        auto count = ctx.run();
        
        // Assert
        REQUIRE(count == 1);
    }
}
```

### 7.2 Contract Violation Tests

```cpp
TEST_CASE("work_queue: contract violations", "[work_queue][contracts]") {
    work_queue<int> queue;
    
    #ifndef NDEBUG
    SECTION("push null handler violates contract") {
        // In Debug, this should trigger assertion
        REQUIRE_THROWS(queue.push(nullptr));
    }
    #endif
}
```

---

## 8. Documentation

### 8.1 Doxygen Comments

```cpp
/**
 * @brief Posts a handler for asynchronous execution.
 * 
 * @param handler Callable to execute. Must not be null.
 * 
 * @pre handler != nullptr
 * @pre !stopped() (can post to stopped context, but won't execute)
 * 
 * @post Handler queued for execution
 * 
 * @thread_safety Thread-safe. Can be called from any thread.
 * 
 * @example
 * @code
 * io_context ctx;
 * ctx.post([] { std::println("Hello from io_context"); });
 * ctx.run();
 * @endcode
 */
template<typename Handler>
void post(Handler&& handler) {
    SVAROG_EXPECTS(handler != nullptr);
    impl_->post(std::forward<Handler>(handler));
}
```

---

## Summary

**Key Principles:**
1. ✅ Use contracts for programming errors, std::expected for runtime errors
2. ✅ Prefer strand over manual locking in async code
3. ✅ Always use smart pointers for ownership
4. ✅ Document thread-safety guarantees
5. ✅ Leverage C++23 features (std::expected, std::move_only_function)
6. ✅ Write comprehensive tests including contract violations

**Questions?** See [REFACTORING_PLAN.md](REFACTORING_PLAN.md) for architecture details.
