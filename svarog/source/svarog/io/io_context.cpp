#include "svarog/io/io_context.hpp"

#include <atomic>
#include <cstddef>
#include <functional>
#include <thread>
#include <utility>

#include "svarog/execution/co_spawn.hpp"

namespace svarog::io {

io_context::io_context([[maybe_unused]] size_t t_concurrency_hint) {
    // Note: m_stopped and m_handlers initialized via in-class initializers
    // concurrency_hint can be used for future optimizations
    // e.g., pre-allocating internal structures
}

void io_context::run() {
    current_context_ = this;
    execution::this_coro::current_context_ = this;

    while (!stopped()) {
        auto handler_result = m_handlers.try_pop();

        if (!handler_result.has_value()) {
            // Queue empty - check if we should exit or wait
            if (m_work_count.load(std::memory_order_acquire) == 0) {
                // No work_guard active - exit naturally
                break;
            }
            // Work guard active but queue empty - yield and retry
            std::this_thread::yield();
            continue;
        }

        // Execute handler
        (*handler_result)();
    }

    current_context_ = nullptr;
    execution::this_coro::current_context_ = nullptr;
}

size_t io_context::run_one() {
    auto handler = m_handlers.try_pop();
    if (handler.has_value()) {
        (*handler)();
        return 1;
    }
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
    return current_context_ == this;
}

execution::schedule_operation io_context::schedule() noexcept {
    return execution::schedule_operation{this};
}

}  // namespace svarog::io
