#pragma once

#include <atomic>
#include <memory>
#include <thread>

#include "svarog/core/contracts.hpp"
#include "svarog/execution/work_queue.hpp"

namespace svarog::execution {
template <typename Executor>
class strand {
public:
    using executor_type = Executor;

    explicit strand(executor_type t_executor);

    ~strand() = default;

    strand(const strand&) = delete;
    strand& operator=(const strand&) = delete;
    strand(strand&&) = delete;
    strand& operator=(strand&&) = delete;

    [[nodiscard]] executor_type get_executor() const noexcept;

    void execute(auto&& t_handler);

    void post(auto&& t_handler);

    void dispatch(auto&& t_handler);

    [[nodiscard]] bool running_in_this_thread() const noexcept;

private:
    void execute_next();

    Executor m_executor;

    std::unique_ptr<work_queue> m_queue;

    std::atomic<bool> m_executing{false};

    std::atomic<std::thread::id> m_running_thread_id;

    thread_local static std::size_t s_execution_depth;
    static constexpr std::size_t max_recursion_depth = 100;
};

template <typename Executor>
thread_local std::size_t strand<Executor>::s_execution_depth = 0;

template <typename Executor>
strand<Executor>::strand(executor_type t_executor)
    : m_executor(std::move(t_executor)), m_queue(std::make_unique<work_queue>()) {
}

template <typename Executor>
typename strand<Executor>::executor_type strand<Executor>::get_executor() const noexcept {
    return m_executor;
}

template <typename Executor>
void strand<Executor>::execute(auto&& t_handler) {
    SVAROG_EXPECTS(t_handler != nullptr);  // Handler must be valid
    post(std::forward<decltype(t_handler)>(t_handler));
}

template <typename Executor>
void strand<Executor>::post(auto&& t_handler) {
    SVAROG_EXPECTS(t_handler != nullptr);  // Handler must be valid

    // Wrap handler in work_item (std::move_only_function<void()>)
    [[maybe_unused]] bool pushed =
        m_queue->push([handler = std::forward<decltype(t_handler)>(t_handler)]() mutable { handler(); });

    // Try to start draining the queue
    // If m_executing was false → we set it to true and start execute_next()
    // If m_executing was true → another thread is already draining, just return
    bool expected = false;
    if (m_executing.compare_exchange_strong(expected, true, std::memory_order_acquire, std::memory_order_relaxed)) {
        // We're the first - schedule execute_next() on underlying executor
        m_executor.execute([this] { execute_next(); });
    }
    // else: Already executing, the running thread will drain our handler
}

template <typename Executor>
void strand<Executor>::dispatch(auto&& t_handler) {
    SVAROG_EXPECTS(t_handler != nullptr);  // Handler must be valid

    if (running_in_this_thread()) {
        // We're already on the strand thread - execute immediately
        // Check recursion depth to prevent stack overflow
        if (s_execution_depth >= max_recursion_depth) {
            // Depth exceeded - defer to post to break recursion
            post(std::forward<decltype(t_handler)>(t_handler));
            return;
        }

        ++s_execution_depth;
        try {
            t_handler();
        } catch (...) {
            --s_execution_depth;
            throw;
        }
        --s_execution_depth;
    } else {
        // Not on strand thread - use post()
        post(std::forward<decltype(t_handler)>(t_handler));
    }
}

template <typename Executor>
bool strand<Executor>::running_in_this_thread() const noexcept {
    return std::this_thread::get_id() == m_running_thread_id.load(std::memory_order_relaxed);
}

template <typename Executor>
void strand<Executor>::execute_next() {
    // Set current thread ID for dispatch optimization
    m_running_thread_id.store(std::this_thread::get_id(), std::memory_order_relaxed);

    // Drain queue until empty
    while (true) {
        auto result = m_queue->try_pop();

        if (!result.has_value()) {
            // Queue empty or stopped - finish execution
            m_running_thread_id.store(std::thread::id{}, std::memory_order_relaxed);
            m_executing.store(false, std::memory_order_release);
            return;
        }

        // Execute handler (catch exceptions to prevent strand termination)
        try {
            (*result)();
        } catch (...) {  // NOLINT(bugprone-empty-catch) - Intentional: strand must continue after handler exceptions
            // Log exception but continue processing
            // In production, should log to error handler
        }
    }
}

}  // namespace svarog::execution
