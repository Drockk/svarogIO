#pragma once

#include <atomic>
#include <functional>

#include "svarog/core/contracts.hpp"
#include "svarog/execution/work_queue.hpp"

namespace svarog::execution {
class executor_work_guard;
}  // namespace svarog::execution

namespace svarog::io {

class io_context {
public:
    class executor_type {
    public:
        void execute(std::move_only_function<void()> t_f) const;
        io_context& context() const noexcept;

        bool operator==(const executor_type& t_other) const noexcept {
            return m_context == t_other.m_context;
        }

    private:
        friend class io_context;
        explicit executor_type(io_context* t_ctx) noexcept : m_context(t_ctx) {
        }
        io_context* m_context;
    };

    explicit io_context(size_t t_concurrency_hint = 0);

    void run();
    size_t run_one();
    void stop();
    bool stopped() const noexcept;
    void restart();
    executor_type get_executor() noexcept;

    void post(auto&& t_handler) {
        [[maybe_unused]] bool pushed = m_handlers.push(std::forward<decltype(t_handler)>(t_handler));
    }

    void dispatch(auto&& t_handler) {
        if (running_in_this_thread()) {
            SVAROG_EXPECTS(!stopped());
            t_handler();
        } else {
            post(std::forward<decltype(t_handler)>(t_handler));
        }
    }

    bool running_in_this_thread() const noexcept;

private:
    std::atomic<bool> m_stopped{false};
    execution::work_queue<> m_handlers;
    std::atomic<size_t> m_work_count{0};
    inline static thread_local io_context* s_current_context = nullptr;

    friend class execution::executor_work_guard;
};

}  // namespace svarog::io
