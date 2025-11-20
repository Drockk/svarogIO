# Coroutine Integration Examples

## Overview

Section 3.7 provides full C++20 coroutine support for `io_context`, enabling modern async programming patterns without callback hell.

## Basic Usage

### Simple Coroutine with schedule()

```cpp
#include "svarog/io/io_context.hpp"
#include "svarog/execution/awaitable_task.hpp"
#include "svarog/execution/co_spawn.hpp"

using namespace svarog;

auto async_task(io::io_context& ctx) -> execution::awaitable_task<int> {
    // Suspend and resume on io_context thread
    co_await ctx.schedule();
    
    // Do some work
    int result = 42;
    
    // Another suspension point
    co_await ctx.schedule();
    
    co_return result;
}

int main() {
    io::io_context ctx;
    
    execution::co_spawn(ctx, async_task(ctx), execution::detached);
    
    ctx.run();
}
```

### Coroutine Composition

```cpp
auto fetch_data(io::io_context& ctx, int id) -> execution::awaitable_task<std::string> {
    co_await ctx.schedule();
    // Simulate async I/O
    co_return "Data for ID: " + std::to_string(id);
}

auto process_data(io::io_context& ctx) -> execution::awaitable_task<void> {
    // Call another coroutine
    std::string data = co_await fetch_data(ctx, 123);
    
    // Process result
    std::cout << data << "\n";
    
    co_return;
}
```

### Multiple Concurrent Coroutines

```cpp
int main() {
    io::io_context ctx;
    std::atomic<int> completed{0};
    
    auto worker = [&](int id) -> execution::awaitable_task<void> {
        for (int i = 0; i < 10; ++i) {
            co_await ctx.schedule();
            // Do work...
        }
        completed++;
        co_return;
    };
    
    // Spawn 4 concurrent coroutines
    for (int i = 0; i < 4; ++i) {
        execution::co_spawn(ctx, worker(i), execution::detached);
    }
    
    // Run until all complete
    auto guard = execution::make_work_guard(ctx);
    std::jthread runner([&]{ ctx.run(); });
    
    while (completed < 4) {
        std::this_thread::sleep_for(10ms);
    }
    
    guard.reset();
    runner.join();
}
```

## Architecture

### awaitable_task<T>

Template coroutine type supporting:
- Return values: `awaitable_task<int>`, `awaitable_task<std::string>`
- Void returns: `awaitable_task<void>`
- Exception propagation
- RAII handle management
- Continuation chain support

### schedule() Awaiter

`io_context::schedule()` returns a `schedule_operation` that:
1. Always suspends (`await_ready() = false`)
2. Posts coroutine handle to `io_context` queue
3. Resumes on next `run()` cycle

### co_spawn() Launcher

Launches coroutines in detached mode:
```cpp
template <typename Awaitable>
void co_spawn(io::io_context&, Awaitable&&, detached_t);
```

Currently supports:
- `detached` token - fire-and-forget execution
- Exception absorption (no std::terminate)

## Performance

Coroutines use the same event loop as callbacks:
- **Zero additional overhead** - same `post()` mechanism
- **No extra allocations** beyond coroutine frame  
- **Benchmarked**: 10.7M ops/sec (same as callback-based `post()`)

## Testing

7 comprehensive tests in `tests/execution/coroutine_tests.cpp`:
1. Basic `awaitable_task` creation and execution
2. Void return type handling
3. `schedule()` awaiter integration
4. `co_spawn()` with detached token
5. Multiple concurrent coroutines
6. Return value propagation
7. Nested coroutine calls

All tests passing with ThreadSanitizer clean.

## Integration with Existing Code

Coroutines and callbacks coexist:

```cpp
io::io_context ctx;

// Callback style
ctx.post([] { std::cout << "Callback\n"; });

// Coroutine style  
auto coro = []() -> execution::awaitable_task<void> {
    co_await ctx.schedule();
    std::cout << "Coroutine\n";
    co_return;
};
execution::co_spawn(ctx, coro(), execution::detached);

// Both execute on same event loop
ctx.run();
```

## Future Enhancements (Phase 2)

Planned additions:
- Cancellation support (`operation_aborted` error code)
- Additional completion tokens (`use_future`, `use_awaitable`)
- Integration with legacy `task<>` API
- Structured concurrency primitives

## References

- C++20 Coroutines: https://en.cppreference.com/w/cpp/language/coroutines
- Boost.Asio coroutine patterns (for comparison)
- `docs/PHASE_1_TASKS.md` - Section 3.7
- `docs/UNIT_TEST_SCENARIOS.md` - Section 2.3
