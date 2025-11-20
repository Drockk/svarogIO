#include "svarog/execution/thread_pool.hpp"

#include <cstddef>
#include <exception>

#include "svarog/core/contracts.hpp"
#include "svarog/execution/work_guard.hpp"

#include <stop_token>

namespace svarog::execution {
thread_pool::thread_pool(size_t t_num_threads) : m_work_guard(make_work_guard(m_context)) {
    SVAROG_EXPECTS(t_num_threads > 0);

    m_threads.reserve(t_num_threads);
    for (size_t i = 0; i < t_num_threads; ++i) {
        m_threads.emplace_back([this](const std::stop_token& t_stoptoken) { worker_thread(t_stoptoken); });
    }
}

thread_pool::~thread_pool() {
    stop();
}

void thread_pool::worker_thread(const std::stop_token& t_stoptoken) {
    while (!t_stoptoken.stop_requested() && !m_context.stopped()) {
        try {
            m_context.run();
            if (m_context.stopped() || t_stoptoken.stop_requested()) {
                break;
            }
            m_context.restart();
        } catch (const std::exception& t_exception) {
            if (t_stoptoken.stop_requested()) {
                break;
            }
        } catch (...) {
            if (t_stoptoken.stop_requested()) {
                break;
            }
        }
    }
}

void thread_pool::stop() noexcept {
    m_work_guard.reset();  // Release work guard to allow run() to exit when empty
    m_context.stop();      // Always stop the context

    for (auto& thread : m_threads) {
        thread.request_stop();
    }
}
}  // namespace svarog::execution
