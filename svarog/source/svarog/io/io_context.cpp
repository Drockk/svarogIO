#include "svarog/io/io_context.hpp"

namespace svarog::io {

io_context::io_context([[maybe_unused]] size_t t_concurrency_hint) {
    // Note: m_stopped and m_handlers initialized via in-class initializers
    // concurrency_hint can be used for future optimizations
    // e.g., pre-allocating internal structures
}

void io_context::run() {
    while (!stopped()) {
        // Use blocking pop() to avoid busy-waiting
        auto handler_result = m_handlers.pop();

        if (!handler_result) {
            // Queue stopped or error - exit loop
            break;
        }

        // Execute handler
        (*handler_result)();

        // Future: Poll I/O events (epoll_wait with timeout)
        // Future: Process I/O completion handlers
        // Future: Check timers
    }
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

void io_context::executor_type::execute(std::move_only_function<void()> f) const {
    // Ignore return value - if push fails, context is stopped and work will not execute
    // This is expected behavior for executor pattern
    [[maybe_unused]] bool pushed = m_context->m_handlers.push(std::move(f));
}

io_context& io_context::executor_type::context() const noexcept {
    return *m_context;
}

}  // namespace svarog::io
