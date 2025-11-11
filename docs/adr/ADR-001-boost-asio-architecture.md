# ADR-001: Boost.Asio Architecture Adoption

**Date:** 2025-11-10  
**Status:** Accepted

## Context

Current svarogIO implementation uses a custom task system based on C++20 coroutines, inspired by the article "C++20 Coroutines Driving a Job System" (poniesandlight.co.uk). The system works, but has the following limitations:

1. **Non-standard architecture** - unique implementation makes onboarding new developers difficult
2. **Lack of synchronization patterns** - every programmer must manually manage mutexes
3. **Future requirements** - planned use for UDP/TCP socket handling requires solid async I/O foundation
4. **Ecosystem** - no ready-made solutions compatible with our architecture

The project goal is to create a production-ready system for handling asynchronous I/O operations, especially for networking (TCP/UDP).

## Decision

**We adopt Boost.Asio-inspired architecture as the foundation for async I/O system.**

We implement the following core components:

### 1. execution_context (base class)
- Manages execution resources (work queue, threads)
- Provides executor interface
- RAII lifecycle management

### 2. io_context (concrete implementation)
- Event loop and thread pool
- Methods: `run()`, `stop()`, `restart()`, `post()`, `dispatch()`
- Compatible with C++23 executors

### 3. strand
- **Key innovation:** implicit serialization without manual locking
- Guarantee: handlers in the same strand never execute in parallel
- Methods: `post()`, `dispatch()`, `running_in_this_thread()`

### 4. work_guard
- RAII object preventing premature io_context termination
- Replaces manual reference counters

## Consequences

### Positive

1. **Recognizability:** Architecture known to anyone who worked with Boost.Asio, standalone Asio, or networking TS
2. **Strand = game changer:**
   - Eliminates 90% of `std::mutex` usage in application code
   - Single-threaded code automatically works in multi-threaded environment
   - Example: each TCP connection has its own strand - zero race conditions
3. **Scalability:** Naturally supports thread pools and load balancing
4. **Testing:** Easy testing - can run io_context in single-threaded mode
5. **Ecosystem:** Hundreds of examples, tutorials, and libraries on the internet
6. **C++23 ready:** Easy migration to `std::execution` in the future

### Negative

1. **Learning curve:** Team must understand Asio programming model (strand, dispatch vs post)
2. **Overhead:** Strand introduces ~5-10% overhead vs direct call (acceptable)
3. **Complexity:** More abstractions than simple task scheduler
4. **Debugging:** Asynchronous call stack can be harder to debug

### Risk Mitigation

- **Learning:** Preparation of documentation and examples (REFACTORING_PLAN.md)
- **Overhead:** Benchmarks confirm acceptable performance
- **Debugging:** observer_ptr and call_stack_marker for call tracking

## Alternatives

### 1. Stay with current system
**Rejected - reasons:**
- No strand = every developer writes their own mutexes (error-prone)
- Difficulties integrating with external async I/O libraries
- Reinventing the wheel

### 2. Use Boost.Asio directly
**Rejected - reasons:**
- Dependency on entire Boost (large footprint)
- Not 100% compatible with C++23
- We want to control implementation for education and customization

### 3. std::execution from C++26
**Rejected - reasons:**
- Standard not ready yet (C++26 = ~2026-2029)
- No reference implementations
- Too early stage for production code

### 4. Custom lockfree with improved locks only
**Rejected - reasons:**
- Doesn't solve the lack of strand problem
- Lockfree != thread-safe code (synchronization still needed)
- Maintainability vs performance trade-off

## References

- [Boost.Asio Documentation](https://www.boost.org/doc/libs/release/doc/html/boost_asio.html)
- [Article: Understanding Asio Strands](https://gamedeveloper.com/programming/a-guide-to-getting-started-with-boost-asio)
- [C++20 Coroutines Job System](https://poniesandlight.co.uk/reflect/c++20_coroutines_driving_a_job_system/)
- Reddit discussion: [Asio io_context mechanics](https://www.reddit.com/r/cpp/comments/1amcp4r/)

## Related Decisions

- [ADR-002](ADR-002-remove-lockfree-ring-buffer-en.md) - Consequence: removing lockfree_ring_buffer
- [ADR-004](ADR-004-cpp23-features-en.md) - C++23 features in implementation

---

**Approved by:** drock  
**Review:** -
