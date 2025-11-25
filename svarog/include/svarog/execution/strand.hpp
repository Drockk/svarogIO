#pragma once

#include <atomic>
#include <memory>
#include <thread>

#include "svarog/core/contracts.hpp"
#include "svarog/execution/work_queue.hpp"

namespace svarog::execution {

// Forward declaration for the shared state
template <typename Executor>
struct strand_state;

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
    static void execute_next(std::shared_ptr<strand_state<Executor>> t_state);

    Executor m_executor;

    std::shared_ptr<strand_state<Executor>> m_state;

    thread_local static std::size_t s_execution_depth;
    static constexpr std::size_t max_recursion_depth = 100;
};

// Shared state that can outlive the strand object
template <typename Executor>
struct strand_state {
    work_queue<> m_queue;
    std::atomic<bool> m_executing{false};
    std::atomic<std::thread::id> m_running_thread_id;
};

template <typename Executor>
thread_local std::size_t strand<Executor>::s_execution_depth = 0;

template <typename Executor>
strand<Executor>::strand(executor_type t_executor)
    : m_executor(std::move(t_executor)), m_state(std::make_shared<strand_state<Executor>>()) {
}

template <typename Executor>
typename strand<Executor>::executor_type strand<Executor>::get_executor() const noexcept {
    return m_executor;
}

template <typename Executor>
void strand<Executor>::execute(auto&& t_handler) {
    post(std::forward<decltype(t_handler)>(t_handler));
}

template <typename Executor>
void strand<Executor>::post(auto&& t_handler) {
    // Wrap handler in work_item (std::move_only_function<void()>)
    [[maybe_unused]] bool pushed =
        m_state->m_queue.push([handler = std::forward<decltype(t_handler)>(t_handler)]() mutable { handler(); });

    // Try to start draining the queue
    // If m_executing was false → we set it to true and start execute_next()
    // If m_executing was true → another thread is already draining, just return
    bool expected = false;
    if (m_state->m_executing.compare_exchange_strong(expected, true, std::memory_order_acquire,
                                                     std::memory_order_relaxed)) {
        // We're the first - schedule execute_next() on underlying executor
        // Capture state by shared_ptr to ensure it outlives the execution
        auto state = m_state;
        m_executor.execute([state] { execute_next(state); });
    }
    // else: Already executing, the running thread will drain our handler
}

template <typename Executor>
void strand<Executor>::dispatch(auto&& t_handler) {
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
    return std::this_thread::get_id() == m_state->m_running_thread_id.load(std::memory_order_relaxed);
}

template <typename Executor>
void strand<Executor>::execute_next(std::shared_ptr<strand_state<Executor>> t_state) {
    // Set current thread ID for dispatch optimization
    t_state->m_running_thread_id.store(std::this_thread::get_id(), std::memory_order_relaxed);

    // Drain queue until empty with proper double-check pattern
    while (true) {
        auto result = t_state->m_queue.try_pop();

        if (result.has_value()) {
            // Execute handler (catch exceptions to prevent strand termination)
            try {
                (*result)();
            } catch (...) {  // NOLINT(bugprone-empty-catch)
                // Intentional: strand must continue after handler exceptions
            }
            continue;
        }

        // Queue appears empty - clear thread ID first
        t_state->m_running_thread_id.store(std::thread::id{}, std::memory_order_relaxed);

        // Release the executing flag - use release semantics
        t_state->m_executing.store(false, std::memory_order_release);

        // Double-check: queue might have been filled between try_pop() and store(false)
        // If queue is not empty, try to reacquire executing flag
        if (t_state->m_queue.empty()) {
            // Queue is still empty - we're done
            return;
        }

        // There might be work! Try to reacquire the executing flag
        bool expected = false;
        if (!t_state->m_executing.compare_exchange_strong(expected, true, std::memory_order_acquire,
                                                          std::memory_order_relaxed)) {
            // Another thread took over - they will handle the queue
            return;
        }

        // We reacquired - restore thread ID and loop to process items
        t_state->m_running_thread_id.store(std::this_thread::get_id(), std::memory_order_relaxed);
        // Loop continues - will try_pop() again
    }
}

}  // namespace svarog::execution
