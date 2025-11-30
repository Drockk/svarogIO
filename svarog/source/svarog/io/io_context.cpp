#include "svarog/io/io_context.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <functional>
#include <utility>

#include "svarog/execution/work_queue.hpp"

namespace svarog::io {

io_context::io_context([[maybe_unused]] size_t t_concurrency_hint) {
    // Note: m_stopped and m_handlers initialized via in-class initializers
    // concurrency_hint can be used for future optimizations
    // e.g., pre-allocating internal structures
}

// TODO: Implement when timer_queue is ready
// void io_context::process_timers() {
//     auto now = std::chrono::steady_clock::now();
//     while (m_timer_queue.has_expired(now)) {
//         auto handler = m_timer_queue.pop_expired();
//         m_handlers.push(std::move(handler));
//     }
// }

// TODO: Implement when timer_queue is ready
// std::chrono::milliseconds io_context::get_next_timer_timeout() const {
//     if (m_timer_queue.empty()) {
//         return std::chrono::milliseconds::max();
//     }
//     auto now = std::chrono::steady_clock::now();
//     auto next_expiry = m_timer_queue.get_next_expiry();
//     if (next_expiry <= now) {
//         return std::chrono::milliseconds::zero();
//     }
//     return std::chrono::duration_cast<std::chrono::milliseconds>(next_expiry - now);
// }

std::chrono::milliseconds io_context::calculate_timeout() const {
    // If work_queue has pending handlers, don't wait
    if (!m_handlers.empty()) {
        return std::chrono::milliseconds::zero();
    }

    // TODO: When timer_queue is implemented, use:
    // auto timer_timeout = get_next_timer_timeout();

    // If we have work_guard, we can wait indefinitely (or until timer)
    if (m_work_count.load(std::memory_order_acquire) > 0) {
        // TODO: return std::min(timer_timeout, std::chrono::milliseconds::max());
        return std::chrono::milliseconds{100};  // Poll every 100ms for now
    }

    // No work_guard and no handlers - don't wait
    return std::chrono::milliseconds::zero();
}

bool io_context::has_pending_work() const noexcept {
    // Has work if:
    // 1. work_queue is not empty, OR
    // 2. work_guard is active (m_work_count > 0), OR
    // 3. reactor has registered descriptors
    // TODO: 4. timer_queue has pending timers

    if (!m_handlers.empty()) {
        return true;
    }

    if (m_work_count.load(std::memory_order_acquire) > 0) {
        return true;
    }

    // TODO: Check reactor for registered I/O operations
    // if (!m_reactor.empty()) {
    //     return true;
    // }

    // TODO: Check timer_queue when implemented
    // if (!m_timer_queue.empty()) {
    //     return true;
    // }

    return false;
}

void io_context::run() {
    s_current_context = this;

    while (!stopped()) {
        // Check exit condition
        if (!has_pending_work()) {
            break;
        }

        // TODO: Process expired timers when timer_queue is implemented
        // process_timers();

        // Calculate how long reactor should wait
        auto timeout = calculate_timeout();

        // Poll reactor for I/O events
        // Reactor will post completion handlers to work_queue internally
        m_reactor.run_one(timeout);

        // Execute all ready handlers from work_queue
        while (true) {
            auto handler_result = m_handlers.try_pop();
            if (!handler_result.has_value()) {
                break;
            }
            (*handler_result)();
        }
    }

    s_current_context = nullptr;
}

size_t io_context::run_one() {
    s_current_context = this;

    // TODO: Process expired timers when timer_queue is implemented
    // process_timers();

    // Try to execute a handler from work_queue first
    auto handler_result = m_handlers.try_pop();
    if (handler_result.has_value()) {
        (*handler_result)();
        s_current_context = nullptr;
        return 1;
    }

    // No handler ready, poll reactor with timeout
    auto timeout = calculate_timeout();
    auto count = m_reactor.run_one(timeout);

    // Check again for handlers (reactor may have posted some)
    handler_result = m_handlers.try_pop();
    if (handler_result.has_value()) {
        (*handler_result)();
        s_current_context = nullptr;
        return 1;
    }

    s_current_context = nullptr;
    return count;
}

size_t io_context::poll() {
    s_current_context = this;
    size_t count = 0;

    // TODO: Process expired timers when timer_queue is implemented
    // process_timers();

    // Poll reactor without blocking
    m_reactor.poll_one();

    // Execute all ready handlers
    while (true) {
        auto handler_result = m_handlers.try_pop();
        if (!handler_result.has_value()) {
            break;
        }
        (*handler_result)();
        ++count;
    }

    s_current_context = nullptr;
    return count;
}

size_t io_context::poll_one() {
    s_current_context = this;

    // TODO: Process expired timers when timer_queue is implemented
    // process_timers();

    // Poll reactor without blocking
    m_reactor.poll_one();

    // Try to execute one handler
    auto handler_result = m_handlers.try_pop();
    if (handler_result.has_value()) {
        (*handler_result)();
        s_current_context = nullptr;
        return 1;
    }

    s_current_context = nullptr;
    return 0;
}

void io_context::stop() {
    m_stopped.store(true, std::memory_order_release);
    // Stop internal work_queue to wake up all blocked run() calls
    m_handlers.stop();
}

bool io_context::stopped() const noexcept {
    return m_stopped.load(std::memory_order_acquire);
}

void io_context::restart() {
    m_handlers.clear();
    m_stopped.store(false, std::memory_order_release);
}

io_context::executor_type io_context::get_executor() noexcept {
    return executor_type(this);
}

void io_context::executor_type::execute(std::move_only_function<void()> t_f) const {
    // Ignore return value - if push fails, context is stopped and work will not execute
    // This is expected behavior for executor pattern
    [[maybe_unused]] const bool is_pushed = m_context->m_handlers.push(std::move(t_f));
}

io_context& io_context::executor_type::context() const noexcept {
    return *m_context;
}

bool io_context::running_in_this_thread() const noexcept {
    return s_current_context == this;
}

}  // namespace svarog::io
