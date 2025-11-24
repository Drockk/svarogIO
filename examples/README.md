# svarogIO Examples

This directory contains practical examples demonstrating how to use the svarogIO library components.

## Building Examples

```bash
# Configure and build
cmake --preset=release
cmake --build --preset=release

# Run all examples
./build/release/examples/work_queue_example/work_queue_example
./build/release/examples/work_guard_example/work_guard_example
./build/release/examples/io_context_example/io_context_example
./build/release/examples/thread_pool_example/thread_pool_example
```

## Examples Overview

### 1. work_queue_example
**File**: `work_queue/main.cpp`

Demonstrates thread-safe MPMC queue:
- Basic push/pop operations
- `try_pop()` for non-blocking access
- Multi-producer multi-consumer pattern
- Queue lifecycle (stop, clear)
- Error handling with `std::expected`

**Key Concepts**:
- FIFO ordering guarantee
- Thread-safe operations
- Producer-consumer patterns
- Graceful shutdown

### 2. work_guard_example
**File**: `work_guard/main.cpp`

Demonstrates RAII lifetime management:
- Preventing premature `run()` exit
- Manual control with `reset()`
- Move semantics
- Multiple guards on same context
- Delayed work posting

**Key Concepts**:
- RAII pattern
- Lifetime management
- Work count tracking
- Move-only semantics

### 3. io_context_example
**File**: `io_context/main.cpp`

Demonstrates event loop and async execution:
- Basic `post()` and `run()`
- `dispatch()` vs `post()` semantics
- Multi-threaded execution
- `run_one()` for step execution
- `stop()` and `restart()`
- Executor pattern

**Key Concepts**:
- Event loop architecture
- Thread-safe work submission
- Immediate vs deferred execution
- Multi-threaded event processing

### 4. thread_pool_example
**File**: `thread_pool/main.cpp`

Demonstrates RAII thread pool management:
- Basic thread pool usage
- CPU-bound work distribution
- Access to underlying `io_context`
- Producer-consumer pattern
- Graceful shutdown
- Automatic RAII cleanup

**Key Concepts**:
- RAII thread management
- Work distribution
- Resource cleanup
- Parallel task execution

## Architecture Overview

```
io_context (event loop)
    ├── work_queue (internal)
    ├── Executor API
    └── work_guard (lifetime helpers)

thread_pool (wrapper)
    └── io_context + std::jthread
```

## Design Patterns

### 1. RAII Pattern
Core components use RAII for resource management:
- `work_guard` - Work count management
- `thread_pool` - Thread lifetime
- `io_context` - Queue ownership and stop/reset semantics

### 2. Executor Pattern
```cpp
auto executor = ctx.get_executor();
executor.execute([]{  });
```

### 3. Thread Pool Pattern
```cpp
execution::thread_pool pool(4);
pool.post([]{ /* work */ });
```

## Performance Characteristics

Based on benchmarks (see `benchmarks/` directory):

| Component | Throughput | Latency |
|-----------|-----------|---------|
| `work_queue` | 18.8M ops/sec | ~53ns |
| `io_context` (1 thread) | 10.7M ops/sec | P50: 412ns |
| `io_context` (4 threads) | 5.4M ops/sec | P99: 3.6µs |

All components are:
- **Lock-free** where possible (`work_queue` uses mutex)
- **Zero-copy** for move-only types
- **Thread-safe** for concurrent access
- **Exception-safe** with strong guarantees

## Common Patterns

### Pattern 1: Event Loop with Work Guard
```cpp
io::io_context ctx;
auto guard = make_work_guard(ctx);

std::jthread worker([&]{ ctx.run(); });

// Post work asynchronously
ctx.post([&]{
    // Do work...
    guard.reset();  // Signal completion
});

worker.join();
```

### Pattern 2: Thread Pool for Parallel Tasks
```cpp
execution::thread_pool pool(4);

for (auto& task : tasks) {
    pool.post([task]{
        process(task);
    });
}

pool.stop();
pool.wait();
```

## Error Handling

All components use modern C++ error handling:
- `std::expected<T, E>` for recoverable errors (`work_queue`)
- `noexcept` for operations that cannot fail
- Exceptions for programming errors (contract violations)
- `SVAROG_EXPECTS/ENSURES` for precondition/postcondition checks

## Thread Safety

| Component | Thread Safety |
|-----------|---------------|
| `work_queue` | MPMC safe |
| `io_context` | All methods thread-safe |
| `work_guard` | Constructor/destructor NOT thread-safe, methods are |
| `thread_pool` | All methods thread-safe |

## Next Steps

1. Read the [Phase 1 Tasks](../docs/PHASE_1_TASKS.md) for implementation details
2. Check [Unit Test Scenarios](../docs/UNIT_TEST_SCENARIOS.md) for usage patterns
3. Review benchmarks in `benchmarks/` for performance characteristics

## License

See [LICENSE.md](../LICENSE.md) for licensing information.
