# ADR-005: Contract Programming with GSL

**Date:** 2025-11-10  
**Status:** Accepted

## Context

Asynchronous programming with io_context, strand, and work_queue introduces complexity:

1. **Complex call chains** - Async handlers execute far from call site, making debugging harder
2. **Thread-safety assumptions** - Code assumes certain threading guarantees (e.g., strand serialization)
3. **API misuse** - Easy to pass null handlers, call methods in wrong state, etc.
4. **Unclear contracts** - What are preconditions? What does the API guarantee?

Traditional approaches:
- **Exceptions** - Too slow, wrong for programming errors
- **Assertions** - Not standardized, inconsistent usage
- **Comments** - Not enforced, become outdated
- **Return codes** - Ignored, verbose

### Example Problems

```cpp
// Problem 1: Null handler causes crash deep in event loop
io_ctx.post(nullptr);  // Crashes later, hard to debug

// Problem 2: Calling run() on stopped context - silent failure
io_ctx.stop();
io_ctx.run();  // What happens? Undefined!

// Problem 3: Strand recursion causes stack overflow
strand.dispatch([&] {
    strand.dispatch([&] {
        strand.dispatch([&] {
            // ... infinite recursion
        });
    });
});
```

## Decision

**We adopt Contract Programming using GSL (Guidelines Support Library) with custom macros:**

### Core Components

1. **GSL-Lite integration** - Header-only, minimal dependency
2. **Custom macros** - `SVAROG_EXPECTS`, `SVAROG_ENSURES`
3. **Build-time control** - Enabled in Debug, disabled in Release (default)
4. **Clear policy** - Contracts for programming errors, std::expected for runtime errors

### Implementation

```cpp
// svarog/include/svarog/core/contracts.hpp
#include <gsl/gsl-lite.hpp>

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

} // namespace svarog::core
```

### Usage Guidelines

**Use SVAROG_EXPECTS for:**
- Null pointer checks
- Invalid state transitions
- Thread-safety violations
- API misuse

**Use std::expected for:**
- Network errors
- File I/O errors
- Expected runtime failures

## Examples

### Before (No Contracts)

```cpp
void io_context::post(std::move_only_function<void()> handler) {
    // Silent failure if handler is null? Crash later?
    impl_->work_queue_.push(std::move(handler));
}

std::size_t io_context::run() {
    // Can we call run() when stopped? No documentation!
    return run_impl();
}
```

### After (With Contracts)

```cpp
void io_context::post(std::move_only_function<void()> handler) {
    SVAROG_EXPECTS(handler != nullptr);  // Clear precondition
    impl_->work_queue_.push(std::move(handler));
}

std::size_t io_context::run() {
    SVAROG_EXPECTS(!stopped());  // Must not be stopped
    
    auto count = run_impl();
    
    SVAROG_ENSURES(stopped());   // Guarantee: stopped after run
    return count;
}
```

### Strand Recursion Protection

```cpp
template<typename F>
void strand::dispatch(F&& f) {
    SVAROG_EXPECTS(f != nullptr);
    
    if (running_in_this_thread()) {
        SVAROG_EXPECTS(execution_depth_ < max_recursion_depth);  // Prevent overflow
        ++execution_depth_;
        f();
        --execution_depth_;
    } else {
        post(std::forward<F>(f));
    }
}
```

## Consequences

### Positive

1. **Early Error Detection**
   - Violations caught at call site in Debug builds
   - Clear error messages with exact location
   - Fail-fast instead of undefined behavior

2. **Executable Documentation**
   - Preconditions and postconditions are enforced
   - API contracts are clear and verified
   - Reduces need for extensive documentation

3. **Zero Overhead in Release**
   - Contracts compiled out by default in Release builds
   - Can be enabled with `-DSVAROG_ENABLE_CONTRACTS_IN_RELEASE`
   - Performance-critical paths unaffected

4. **Better Than Alternatives**
   - More structured than ad-hoc assertions
   - Clearer than exceptions for programming errors
   - More enforceable than comments

5. **Future-Proof**
   - Bridge to C++26 contracts standard
   - Easy migration when standard is ready
   - GSL is widely adopted

6. **Thread-Safety Validation**
   - Verify threading assumptions at runtime
   - Catch strand violations early
   - Document thread-safety guarantees

### Negative

1. **Learning Curve**
   - Team must understand when to use contracts vs std::expected
   - New concept for developers unfamiliar with Design by Contract
   - Requires discipline to use consistently

2. **Dependency on GSL**
   - Additional dependency (though header-only)
   - Need to keep GSL-Lite updated
   - Alternative: wait for C++26 (too far away)

3. **Debug Build Overhead**
   - Contract checks add overhead in Debug builds
   - May need to disable in performance-sensitive tests
   - Mitigated by selective contract placement

4. **Not a Silver Bullet**
   - Doesn't replace testing
   - Can't check all invariants efficiently
   - Complex invariants still need comments

### Risk Mitigation

- **Clear Guidelines** - Created CODING_STANDARDS.md with usage policy
- **Training** - Examples in all Phase 1 components
- **Review Process** - Code reviews enforce proper usage
- **Selective Use** - Hot paths use minimal contracts

## Alternatives

### 1. Plain assertions

**Rejected - Reasons:**
```cpp
assert(handler != nullptr);  // Not standardized, inconsistent
```
- No precondition/postcondition distinction
- Platform-specific behavior
- Less clear intent

### 2. Exceptions for everything

**Rejected - Reasons:**
```cpp
if (!handler) throw std::invalid_argument("null handler");
```
- Performance overhead
- Wrong semantics (programming errors aren't exceptions)
- Forces error handling for bugs

### 3. std::expected for everything

**Rejected - Reasons:**
```cpp
std::expected<void, Error> post(Handler&& h) {
    if (!h) return std::unexpected(Error::NullHandler);
    // ...
}
```
- Verbose for programming errors
- Caller can ignore errors
- Wrong semantics (bugs should terminate, not be handled)

### 4. Wait for C++26 contracts

**Rejected - Reasons:**
- C++26 standard not finalized (2026-2029 timeframe)
- No production-ready implementations yet
- Project needs contracts now for Phase 1

### 5. Custom assertion framework

**Rejected - Reasons:**
- Reinventing the wheel
- More maintenance burden
- GSL is standard, well-tested, widely used

## Implementation Plan

1. **Phase 0** (before Phase 1):
   - Integrate GSL-Lite via CMake FetchContent
   - Create `svarog/include/svarog/core/contracts.hpp`
   - Write `docs/CODING_STANDARDS.md` with guidelines

2. **Phase 1** (during implementation):
   - Add contracts to execution_context
   - Add contracts to work_queue
   - Add contracts to io_context
   - Add contracts to strand

3. **Validation**:
   - Write tests for contract violations
   - Verify zero overhead in Release builds
   - Document all public API contracts

## Metrics for Success

- ✅ All Phase 1 public APIs have documented contracts
- ✅ Zero contract violations in unit tests
- ✅ No performance regression in Release builds
- ✅ Contract violations caught early in Debug builds
- ✅ Clear violation messages aid debugging

## References

- [GSL-Lite](https://github.com/gsl-lite/gsl-lite) - Implementation we're using
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines) - Contract philosophy
- [P2900R6 - Contracts for C++](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2900r6.pdf) - Future standard
- [Herb Sutter - De-fragmenting C++](https://www.youtube.com/watch?v=ARYP83yNAWk) - Contracts discussion

## Related Decisions

- [ADR-001](ADR-001-boost-asio-architecture.md) - Asio architecture provides context for contract usage
- [ADR-004](ADR-004-cpp23-features.md) - C++23 features complement contracts (std::expected)
- [CODING_STANDARDS.md](../CODING_STANDARDS.md) - Detailed usage guidelines

---

**Approved by:** drock  
**Review:** Phase 1 team (pending)
