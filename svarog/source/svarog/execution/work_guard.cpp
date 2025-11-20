#include "svarog/execution/work_guard.hpp"

#include <atomic>
#include <utility>  // std::exchange

#include "svarog/io/io_context.hpp"

#include <gsl/gsl>

namespace svarog::execution {

executor_work_guard::executor_work_guard(io::io_context& t_ctx) noexcept : m_context(&t_ctx) {
    m_context->m_work_count.fetch_add(1, std::memory_order_relaxed);
}

executor_work_guard::~executor_work_guard() {
    reset();
}

executor_work_guard::executor_work_guard(executor_work_guard&& t_other) noexcept
    : m_context(std::exchange(t_other.m_context, nullptr)) {
}

executor_work_guard& executor_work_guard::operator=(executor_work_guard&& t_other) noexcept {
    if (this != &t_other) {
        reset();
        m_context = std::exchange(t_other.m_context, nullptr);
    }
    return *this;
}

void executor_work_guard::reset() noexcept {
    if (m_context != nullptr) {
        m_context->m_work_count.fetch_sub(1, std::memory_order_release);
        m_context->m_handlers.notify_all();  // Wake up any blocked run() calls
        m_context = nullptr;
    }
}

io::io_context& executor_work_guard::get_executor() const noexcept {
    Expects(m_context != nullptr);
    return *m_context;
}

}  // namespace svarog::execution
