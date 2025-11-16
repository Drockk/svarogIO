#pragma once

#include <atomic>

namespace svarog::io {
class io_context;
}

namespace svarog::execution {
///
/// A work_guard prevents an io_context from exiting run() when the work queue
/// is empty. This is useful when you want to keep the io_context alive while
/// waiting for asynchronous operations to post more work.
///
/// @par Thread Safety:
/// - Constructor and destructor are NOT thread-safe (single ownership)
/// - reset() is thread-safe
/// - get_executor() is thread-safe
///
/// @par Example Usage:
/// @code
/// io_context ctx;
///
/// // Create work_guard to keep ctx.run() alive
/// auto guard = make_work_guard(ctx);
///
/// std::jthread worker([&]{ ctx.run(); });
///
/// // Post async work
/// ctx.post([&]{
///     // Do work...
///     guard.reset();  // Release guard when done
/// });
///
/// worker.join();  // run() exits after guard released and queue empty
/// @endcode
///
/// @note Modeled after boost::asio::executor_work_guard
class executor_work_guard {
public:
    /// @brief Construct work_guard for an io_context.
    ///
    /// @param ctx io_context to guard
    ///
    /// @post ctx.run() will not exit while this guard exists
    explicit executor_work_guard(io::io_context& ctx) noexcept;

    /// @brief Destructor - releases the work guard.
    ///
    /// @post If this was the last guard, io_context may exit run()
    ~executor_work_guard();

    // Non-copyable
    executor_work_guard(const executor_work_guard&) = delete;
    executor_work_guard& operator=(const executor_work_guard&) = delete;

    // Movable
    executor_work_guard(executor_work_guard&& other) noexcept;
    executor_work_guard& operator=(executor_work_guard&& other) noexcept;

    /// @brief Release the work guard.
    ///
    /// @post io_context may exit run() if no other guards or work exist
    ///
    /// @note Can be called multiple times (idempotent)
    /// @thread_safety Thread-safe
    void reset() noexcept;

    /// @brief Check if guard is still active.
    ///
    /// @return true if guard is holding work count, false otherwise
    /// @thread_safety Thread-safe
    bool owns_work() const noexcept {
        return m_context != nullptr;
    }

    /// @brief Get the associated io_context.
    ///
    /// @return Reference to the io_context
    /// @pre owns_work() == true
    io::io_context& get_executor() const noexcept;

private:
    io::io_context* m_context{nullptr};
};

/// @brief Create a work_guard for an io_context.
///
/// @param ctx io_context to guard
/// @return work_guard object
///
/// @par Example:
/// @code
/// io_context ctx;
/// auto guard = make_work_guard(ctx);
/// @endcode
inline executor_work_guard make_work_guard(io::io_context& ctx) noexcept {
    return executor_work_guard(ctx);
}

}  // namespace svarog::execution
