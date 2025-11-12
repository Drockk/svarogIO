#pragma once

#include <atomic>
#include <functional>      // std::move_only_function
#include <memory>          // std::unique_ptr
#include <optional>        // std::optional for try_pop
#include <cstddef>         // size_t
#include "svarog/core/contracts.hpp"

namespace svarog::execution {

/// Work item type - move-only callable with no return value
/// @note Uses C++23 std::move_only_function for zero-overhead handlers
/// @note Allows capturing unique resources (std::unique_ptr, etc.)
using work_item = std::move_only_function<void()>;

/// Thread-safe MPMC work queue for asynchronous task execution
///
/// @brief Lock-free queue optimized for multi-producer multi-consumer workloads
///
/// The work_queue provides a thread-safe mechanism for distributing work items
/// across multiple worker threads. It uses lock-free atomic operations to ensure
/// high throughput and low latency in concurrent scenarios.
///
/// @invariant Queue operations are wait-free or lock-free
/// @invariant work_item destruction happens on consumer thread or in destructor
/// @invariant MPMC safety: Multiple producers and consumers can operate concurrently
/// @invariant FIFO ordering: Items processed in push order (best-effort under contention)
/// @invariant Stopped state: Once stopped, push() always fails
///
/// @threadsafety All methods are thread-safe
/// @threadsafety push() and try_pop() can be called from multiple threads concurrently
/// @threadsafety size() and empty() provide approximate results only
/// @threadsafety stop() synchronizes with all threads
///
/// @par Example Usage:
/// @code
/// work_queue queue(1024);
/// 
/// // Producer thread
/// queue.push([] { std::cout << "Task 1\n"; });
/// queue.push([] { std::cout << "Task 2\n"; });
/// 
/// // Consumer thread
/// while (auto item = queue.try_pop()) {
///     (*item)();  // Execute work item
/// }
/// 
/// // Graceful shutdown
/// queue.stop();
/// @endcode
///
/// @see work_item
/// @see execution_context
class work_queue {
public:
    /// Default capacity hint (implementation-defined)
    /// @note Actual capacity may vary based on implementation
    static constexpr size_t default_capacity = 1024;

    /// Construct work queue with optional capacity hint
    ///
    /// @param capacity Maximum number of items (0 = unlimited if supported)
    ///
    /// @pre capacity == 0 || capacity >= 16
    ///
    /// @post Queue is empty and not stopped
    /// @post Queue ready to accept work items
    ///
    /// @throws std::bad_alloc If memory allocation fails
    ///
    /// @note Capacity is a hint; implementation may round up for alignment
    /// @note Zero capacity may fall back to default_capacity in some implementations
    explicit work_queue(size_t capacity = default_capacity);

    /// Destructor - drains remaining items
    ///
    /// @post All work items destroyed
    /// @post All resources released
    ///
    /// @note Remaining work items are destroyed without execution
    /// @note Destructor does NOT call stop() - caller responsibility
    ~work_queue();

    /// Non-copyable (owns unique resources)
    work_queue(const work_queue&) = delete;
    /// Non-copyable (owns unique resources)
    work_queue& operator=(const work_queue&) = delete;
    
    /// Non-movable (thread resources tied to this instance)
    work_queue(work_queue&&) = delete;
    /// Non-movable (thread resources tied to this instance)
    work_queue& operator=(work_queue&&) = delete;

private:
    /// PIMPL idiom - hides lock-free implementation details
    /// @note Implementation may use Boost.Lockfree, custom ring buffer, or hybrid approach
    struct impl;
    std::unique_ptr<impl> m_impl;
};

} // namespace svarog::execution
