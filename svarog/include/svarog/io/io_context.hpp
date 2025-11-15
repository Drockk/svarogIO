#pragma once

#include <atomic>
#include <functional>

#include "svarog/execution/execution_context.hpp"
#include "svarog/execution/work_queue.hpp"

namespace svarog::io {

/// @brief Event-driven execution context for asynchronous I/O operations.
///
/// io_context provides an event loop for executing asynchronous handlers.
/// Unlike thread pools, io_context requires explicit run() calls to process work.
/// Multiple threads can call run() concurrently to achieve parallelism.
///
/// @par Architecture:
/// - Event loop (not a thread pool)
/// - Uses work_queue internally to store posted handlers
/// - Threads provided by user calling run()
/// - Prepared for epoll/kqueue integration (Phase 2)
///
/// @invariant Handlers executed in FIFO order within single thread
/// @invariant stop() interrupts all run() calls within bounded time
/// @invariant Multiple threads can call run() concurrently for parallel execution
///
/// @threadsafety All methods are thread-safe
/// @threadsafety Multiple threads can call run() concurrently
///
/// @par Example Usage:
/// @code
/// io_context ctx;
///
/// // Post work from any thread
/// auto ex = ctx.get_executor();
/// ex.execute([] { std::cout << "Hello\n"; });
///
/// // Run event loop (blocks until stopped or no work)
/// ctx.run();
///
/// // Multi-threaded execution:
/// std::jthread t1([&]{ ctx.run(); });
/// std::jthread t2([&]{ ctx.run(); });
/// // Both threads process handlers concurrently
/// @endcode
class io_context : public execution::execution_context {
public:
    /// @brief Executor type for submitting work to this io_context.
    ///
    /// Lightweight handle that can be copied. All executors from the same
    /// io_context compare equal and submit work to the same queue.
    ///
    /// @threadsafety All methods are thread-safe
    class executor_type {
    public:
        /// @brief Submit a function for execution on the io_context.
        /// @param f Function to execute (must be invocable with no arguments)
        /// @note Thread-safe, can be called from any thread
        void execute(std::move_only_function<void()> f) const;

        /// @brief Get reference to the associated io_context.
        /// @return Reference to the io_context this executor belongs to
        io_context& context() const noexcept;

        /// @brief Compare executors for equality.
        /// @param other Another executor to compare with
        /// @return true if both executors refer to the same io_context
        /// @note All executors from the same io_context compare equal
        bool operator==(const executor_type& other) const noexcept {
            return m_context == other.m_context;
        }

    private:
        friend class io_context;
        explicit executor_type(io_context* ctx) noexcept : m_context(ctx) {
        }
        io_context* m_context;
    };

    /// @brief Construct io_context with optional concurrency hint.
    ///
    /// @param t_concurrency_hint Hint for number of threads that will call run() (0 = auto)
    ///
    /// @post stopped() == false
    /// @post No handlers are queued
    /// @post Internal work_queue initialized
    ///
    /// @note concurrency_hint is optimization hint, not thread count limit
    /// @note May be used to pre-allocate internal structures in future
    /// @note Default value (0) lets implementation choose optimal settings
    explicit io_context(size_t t_concurrency_hint = 0);

    /// @brief Run the event loop until stopped or no more work.
    ///
    /// Blocks calling thread and executes handlers until stop() is called
    /// or the work queue is empty. Uses blocking pop() to avoid busy-waiting.
    ///
    /// @post All queued handlers executed or stop() was called
    ///
    /// @note Thread-safe, multiple threads can call run() concurrently
    /// @note Returns when stopped() == true or no handlers remain
    /// @note Does NOT busy-wait - yields CPU when no work available
    void run();

    /// @brief Execute at most one handler.
    ///
    /// @return Number of handlers executed (0 or 1)
    ///
    /// @note Thread-safe
    /// @note Non-blocking, returns immediately if no work available
    size_t run_one();

    /// @brief Stop the event loop.
    ///
    /// @post stopped() == true
    /// @post All run() calls will return
    /// @post Internal work_queue stopped
    ///
    /// @note Thread-safe, atomic operation
    /// @note Wakes up all threads blocked in run()
    void stop() override;

    /// @brief Check if event loop has been stopped.
    ///
    /// @return true if stop() has been called, false otherwise
    ///
    /// @note Thread-safe, atomic load
    bool stopped() const noexcept override;

    /// @brief Restart the event loop after stop().
    ///
    /// @pre stopped() == true
    /// @post stopped() == false
    /// @post All pending handlers cleared
    /// @post Ready to accept new work
    ///
    /// @note Thread-safe
    void restart() override;

    /// @brief Get an executor for this io_context.
    ///
    /// @return Executor that can be used to submit work
    ///
    /// @note All executors from same io_context are equivalent
    /// @note Executor remains valid as long as io_context exists
    executor_type get_executor() noexcept;

private:
    std::atomic<bool> m_stopped{false};
    execution::work_queue m_handlers;
};

}  // namespace svarog::io
