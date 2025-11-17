#include "svarog/execution/thread_pool.hpp"

#include <cstddef>
#include <exception>

#include "svarog/core/contracts.hpp"

#include <stop_token>

namespace svarog::execution {
thread_pool::thread_pool(size_t t_num_threads) {
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
            if (m_context.stopped()) {
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
    if (!m_context.stopped()) {
        m_context.stop();
    }

    for (auto& thread : m_threads) {
        thread.request_stop();
    }
}

void thread_pool::wait() {
    for (auto& thread : m_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}
}  // namespace svarog::execution
