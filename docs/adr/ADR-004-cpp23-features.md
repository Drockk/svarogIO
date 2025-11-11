# ADR-004: C++23 Features Integration

**Date:** 2025-11-10  
**Status:** Accepted

## Context

svarogIO project uses C++20 as baseline (coroutines, concepts, modules). C++23 standard introduces several new features that could improve the code. Question: **which C++23 features are worth adopting now?**

### Requirements

1. **Availability:** Feature must be implemented in GCC 13+, Clang 16+, MSVC 19.36+
2. **Value:** Concrete benefit for async I/O architecture
3. **Stability:** Prefer features already in final C++23 draft (not experimental)
4. **Maintainability:** Feature simplifies code, doesn't complicate it

## Decision

**We adopt the following C++23 features in the project:**

### 1. std::expected<T, E> - Priority: 🔴 HIGH

**Use Case:** Error handling without exceptions in hot paths.

**After (C++23):**
```cpp
#include <expected>

enum class queue_error {
    empty,
    shutdown,
    timeout
};

auto work_queue::try_pop() -> std::expected<T, queue_error> {
    std::lock_guard lock(mutex_);
    if (queue_.empty()) {
        return std::unexpected(queue_error::empty);
    }
    T item = queue_.front();
    queue_.pop_front();
    return item;
}

// Usage
if (auto result = queue.try_pop()) {
    process(result.value());
} else {
    handle_error(result.error());
}
```

**Benefits:**
- Monadic operations: `.and_then()`, `.or_else()`, `.transform()`
- Type-safe: compiler enforces error checking
- Zero overhead: optional + variant implementation

**Availability:** GCC 12+, Clang 16+, MSVC 19.33+

---

### 2. std::move_only_function - Priority: 🔴 HIGH

**Use Case 1:** Handlers with unique ownership (capturing unique_ptr).

**After (C++23):**
```cpp
#include <functional>

class io_context {
    std::move_only_function<void()> handler_; // Move-only OK!
};

// Clean: unique ownership
io_ctx.post([data = std::make_unique<large_data>()]() {
    process(*data);
});
```

**Use Case 2:** Service Registry Lifecycle Management

**After (C++23):**
```cpp
class execution_context {
    // Service registry with type erasure
    std::unordered_map<std::type_index, std::shared_ptr<void>> m_services;
    
    // Cleanup callbacks - move_only_function enables zero-overhead RAII
    std::vector<std::move_only_function<void()>> m_cleanup_callbacks;
    
    template<typename Service, typename... Args>
    Service& make_service(Args&&... args) {
        auto service = std::make_shared<Service>(std::forward<Args>(args)...);
        Service& service_ref = *service;  // Get ref BEFORE move
        
        // Cleanup callback captures service by value (shared ownership)
        m_cleanup_callbacks.emplace_back([service = std::move(service)]() mutable {
            if constexpr (HasShutdownHook<Service>) {
                service->on_shutdown();
            }
            service.reset();  // Explicit destruction
        });
        
        return service_ref;
    }
};
```

**Benefits:**
- Zero overhead: no ref counting for callbacks themselves
- RAII semantics: clear ownership in cleanup lambdas
- Larger SBO buffer (typically 32 bytes vs 16 for std::function)
- **Unified storage:** Single vector replaces multiple data structures
- **Move-only captures:** Lambdas can capture unique resources

**Key Advantage over std::function:**

Before (would require shared_ptr for std::function):
```cpp
// std::function requires copyable - forces shared_ptr overhead everywhere
std::vector<std::function<void()>> callbacks;  // ❌ Requires copying
```

After (move-only semantics):
```cpp
// move_only_function allows move-only captures directly
std::vector<std::move_only_function<void()>> callbacks;  // ✅ Zero overhead
callbacks.emplace_back([ptr = std::make_unique<T>()]() { /* ... */ });
```

**Availability:** GCC 12+, Clang 16+, MSVC 19.32+

---

### 3. Deducing this - Priority: 🟡 MEDIUM

**Use Case:** Reduce method duplication (const/non-const overloads).

**After (C++23):**
```cpp
class work_queue {
    Container queue_;
    
    // Single implementation
    template<typename Self>
    auto front(this Self&& self) -> decltype(auto) {
        return std::forward<Self>(self).queue_.front();
    }
};
```

**Benefits:**
- DRY: one implementation instead of two/four
- Perfect forwarding built-in
- Easier CRTP patterns

**Availability:** GCC 14+, Clang 18+, MSVC 19.37+

---

### 4. std::print / std::println - Priority: 🟡 MEDIUM

**Use Case:** Type-safe logging and debugging.

**After (C++23):**
```cpp
#include <print>

std::println("Handler executed on thread {} with latency {}ns",
             std::this_thread::get_id(), latency.count());
```

**Benefits:**
- Type-safe: format string checked at compile-time
- Readable: Python-like syntax
- Fast: compiled format strings

**Availability:** GCC 14+, Clang 17+, MSVC 19.37+

---

### 5. std::flat_map / std::flat_set - Priority: 🟢 LOW

**Use Case:** Small maps/sets with better cache locality.

**After (C++23):**
```cpp
#include <flat_map>

std::flat_map<int, strand*> connection_strands_; // Contiguous storage
```

**Benefits:**
- Cache-friendly: contiguous memory
- Faster iteration: ~3x vs std::map for small collections
- Lower memory overhead: no node pointers

**Availability:** GCC 14+, Clang 18+ (partial), MSVC - **NOT YET**

**Status:** Optional - will use when all compilers support.

---

### 6. std::stacktrace - Priority: 🟢 LOW

**Use Case:** Debugging async call chains.

```cpp
#include <stacktrace>

void strand::post(std::move_only_function<void()> handler) {
    if constexpr (SVAROG_DEBUG) {
        auto trace = std::stacktrace::current();
        std::println("Handler posted from:\n{}", trace);
    }
    // ...
}
```

**Benefits:**
- Built-in stack traces (no need for external libs)
- Helpful for debugging async code

**Availability:** GCC 12+, Clang 18+, MSVC 19.36+

---

## Adoption Timeline

### Phase 1: Immediate (C++23 features we use now)
- ✅ **std::expected<T, E>** - work_queue API
- ✅ **std::move_only_function** - handler storage
- ✅ **std::print/println** - logging (debug builds)

### Phase 2: When Compiler Support Stable (6-12 months)
- ⏳ **Deducing this** - method deduplication
- ⏳ **std::stacktrace** - debugging tools

### Phase 3: Future (when all compilers support)
- 🔮 **std::flat_map/flat_set** - container optimizations
- 🔮 **if consteval** - meta-programming

## Consequences

### Positive

1. **std::expected:** Type-safe error handling, monadic operations, zero-overhead
2. **std::move_only_function:** 
   - Clean unique ownership, no shared_ptr overhead for handlers
   - Enables zero-overhead RAII patterns in service registry
   - Unified storage for cleanup callbacks (single vector replaces multiple structures)
   - Move-only lambda captures without heap allocation
3. **Modern C++:** More readable code, less boilerplate, future-proof

### Negative

1. **Compiler Requirements:** Minimum GCC 13, Clang 16, MSVC 19.33
   - **Mitigation:** Most distros already have it (Ubuntu 23.04+, Fedora 38+)
2. **Learning Curve:** Team must learn new APIs
   - **Mitigation:** Documentation + examples in codebase

## Example: Async Chain with std::expected

**After (monadic operations):**
```cpp
read_socket()
    .and_then([](auto data) { return parse_data(data); })
    .and_then([](auto parsed) { return process(parsed); })
    .or_else([](auto error) { log_error(error); });
```

## Compiler Support Matrix

| Feature | GCC | Clang | MSVC | Status |
|---------|-----|-------|------|--------|
| std::expected | 12+ | 16+ | 19.33+ | ✅ Use Now |
| std::move_only_function | 12+ | 16+ | 19.32+ | ✅ Use Now |
| std::print/println | 14+ | 17+ | 19.37+ | ✅ Use Now |
| Deducing this | 14+ | 18+ | 19.37+ | ⏳ Wait 6 months |
| std::flat_map | 14+ | 18+* | ❌ | ⏳ Wait for MSVC |
| std::stacktrace | 12+ | 18+ | 19.36+ | ⏳ Wait for stable |

## Alternatives

### 1. Stay with C++20
**Rejected - reasons:**
- std::expected is game-changer for error handling
- std::move_only_function eliminates shared_ptr overhead
- Compiler support already sufficient (2024)

### 2. Use all C++23 features
**Rejected - reasons:**
- Some features not yet stable (std::flat_map in MSVC)
- Not all have concrete benefits for this project

### 3. External libraries (Abseil, Folly)
**Rejected - reasons:**
- std::expected/move_only_function already in stdlib
- Want to minimize dependencies

## References

- [C++23 Features on cppreference](https://en.cppreference.com/w/cpp/23)
- [std::expected Tutorial](https://www.sandordargo.com/blog/2022/12/14/cpp23-expected)

## Related Decisions

- [ADR-001](ADR-001-boost-asio-architecture-en.md) - std::move_only_function for handlers
- [ADR-002](ADR-002-remove-lockfree-ring-buffer-en.md) - std::expected in work_queue API
- [ADR-005](ADR-005-contract-programming.md) - Contracts complement std::expected for error handling

## Implementation Notes

### std::move_only_function in execution_context

The service registry implementation (completed 2025-11-11) demonstrates the power of `std::move_only_function` for lifecycle management:

**Pattern:** Type erasure + RAII cleanup
```cpp
// Service stored as void* with type_index key
std::unordered_map<std::type_index, std::shared_ptr<void>> m_services;

// Cleanup callbacks with zero-overhead move semantics
std::vector<std::move_only_function<void()>> m_cleanup_callbacks;
```

**Key Implementation Details:**
1. **Reverse destruction:** Cleanup callbacks executed in reverse order (LIFO)
2. **Shutdown hooks:** Optional `on_shutdown()` method detected via C++23 concepts
3. **Memory safety:** Reference obtained before move to avoid dangling references
4. **Thread safety:** Mutex-protected service registration

This pattern will be reused in `io_context`, `strand`, and `work_queue` for consistent lifecycle management across the library.

---

**Approved by:** drock  
**Implementation Status:** Phase 1 - std::expected and std::move_only_function in use (execution_context complete)
**Last Updated:** 2025-11-11
