#pragma once

#include <thread>
#include <vector>

#include "svarog/io/io_context.hpp"

namespace svarog::execution {
class thread_pool {
public:
    explicit thread_pool(size_t t_num_threads = std::thread::hardware_concurrency());
    ~thread_pool();

    thread_pool(const thread_pool&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;
    thread_pool(thread_pool&&) = delete;
    thread_pool& operator=(thread_pool&&) = delete;

    auto& context() noexcept {
        return m_context;
    }

    auto get_executor() noexcept {
        return m_context.get_executor();
    }

    void post(auto&& t_item) {
        m_context.post(std::forward<decltype(t_item)>(t_item));
    }

    void stop() noexcept;

    [[nodiscard]] bool stopped() const noexcept {
        return m_context.stopped();
    }

    [[nodiscard]] size_t thread_count() const noexcept {
        return m_threads.size();
    }

    void wait();

    // TODO: Implement when strands will be ready
    // auto make_strand() {
    //     return {};
    // }

private:
    io::io_context m_context;
    std::vector<std::jthread> m_threads;

    void worker_thread(const std::stop_token& t_stoptoken);
};
}  // namespace svarog::execution
