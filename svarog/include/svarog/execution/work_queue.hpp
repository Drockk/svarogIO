#pragma once

#include <atomic>
#include <cstddef>     // size_t
#include <functional>  // std::move_only_function
#include <memory>      // std::unique_ptr

#include "svarog/core/contracts.hpp"

#include <expected>  // std::expected, std::unexpect

class work_queue_impl;

namespace svarog::execution {

/// Work item type - move-only callable with no return value
/// @note Uses C++23 std::move_only_function for zero-overhead handlers
/// @note Allows capturing unique resources (std::unique_ptr, etc.)
using work_item = std::move_only_function<void()>;

/// Error codes for work_queue operations
enum class queue_error {
    empty,   ///< Queue is empty
    stopped  ///< Queue is stopped
};

/// Thread-safe MPMC work queue for asynchronous task execution
///
/// @brief Mutex-protected queue optimized for multi-producer multi-consumer workloads
///
/// The work_queue provides a thread-safe mechanism for distributing work items
/// across multiple worker threads. It uses mutex-based synchronization to ensure
/// safe concurrent access and FIFO ordering guarantees.
///
/// @invariant work_item destruction happens on consumer thread or in destructor
/// @invariant MPMC safety: Multiple producers and consumers can operate concurrently
/// @invariant FIFO ordering: Items processed in exact push order
/// @invariant Stopped state: Once stopped, push() always fails
/// @invariant Thread-safety: All operations protected by internal mutex
///
/// @threadsafety All methods are thread-safe
/// @threadsafety push() and try_pop() can be called from multiple threads concurrently
/// @threadsafety size() and empty() return exact results (snapshot under lock)
/// @threadsafety stop() synchronizes with all threads via atomic flag
///
/// @par Example Usage:
/// @code
/// work_queue queue;
///
/// // Producer thread
/// queue.push([] { std::cout << "Task 1\n"; });
/// queue.push([] { std::cout << "Task 2\n"; });
///
/// // Consumer thread
/// while (auto result = queue.try_pop()) {
///     if (result) {
///         (*result)();  // Execute work item
///     } else if (result.error() == queue_error::stopped) {
///         break;  // Queue stopped
///     }
///     // queue_error::empty - try again
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
    /// Construct work queue
    ///
    /// @post Queue is empty and not stopped
    /// @post Queue ready to accept work items
    ///
    /// @throws std::bad_alloc If memory allocation fails
    ///
    /// @note Current implementation uses std::deque with dynamic capacity
    explicit work_queue();

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

    /// Push work item to queue (non-blocking)
    ///
    /// @param t_item Work item to execute (must not be null)
    ///
    /// @return true if item was pushed, false if queue is stopped
    ///
    /// @pre t_item != nullptr
    ///
    /// @post If returns true, item moved into queue
    /// @post If returns false and stopped(), item unchanged
    ///
    /// @note Thread-safe, can be called from multiple threads
    /// @note Always fails if queue is stopped
    [[nodiscard]] bool push(work_item&& t_item);

    /// Try to pop work item from queue (non-blocking)
    ///
    /// @return work_item on success, queue_error on failure
    ///
    /// @post If returns work_item, queue size decreased by 1
    /// @post If returns error, queue unchanged
    ///
    /// @note Thread-safe, can be called from multiple threads
    /// @note Returns queue_error::empty if queue is empty
    /// @note Returns queue_error::stopped if queue is stopped
    [[nodiscard]] std::expected<work_item, queue_error> try_pop() noexcept;

    /// Get current queue size
    ///
    /// @return Number of items currently in queue
    ///
    /// @note Thread-safe, returns exact snapshot
    /// @note Result is accurate at time of call but may change immediately
    [[nodiscard]] size_t size() const noexcept;

    /// Check if queue is empty
    ///
    /// @return true if queue contains no items
    ///
    /// @note Thread-safe, returns exact snapshot
    /// @note Result is accurate at time of call but may change immediately
    [[nodiscard]] bool empty() const noexcept;

    /// Stop accepting new work items
    ///
    /// @post stopped() returns true
    /// @post push() will return false
    /// @post try_pop() will return queue_error::stopped
    ///
    /// @note Thread-safe, atomic operation
    /// @note Idempotent - safe to call multiple times
    /// @note Does NOT drain existing items - use try_pop() to drain
    void stop() noexcept;

    /// Check if queue is stopped
    ///
    /// @return true if stop() has been called
    ///
    /// @note Thread-safe, atomic load
    [[nodiscard]] bool stopped() const noexcept;

private:
    /// PIMPL idiom - hides implementation details
    /// @note Current implementation: std::deque with mutex protection (ADR-002)
    /// @note Future: May optimize to lock-free queue if benchmarks show need
    std::unique_ptr<work_queue_impl> m_impl;
};

}  // namespace svarog::execution
