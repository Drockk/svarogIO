#pragma once

#include <atomic>
#include <functional>

#include "svarog/execution/execution_context.hpp"
#include "svarog/execution/work_queue.hpp"

#include <coroutine>

namespace svarog::execution {
class executor_work_guard;
struct schedule_operation;
}  // namespace svarog::execution

namespace svarog::io {

class io_context : public execution::execution_context<> {
public:
    class executor_type {
    public:
        void execute(std::move_only_function<void()> f) const;
        io_context& context() const noexcept;

        bool operator==(const executor_type& other) const noexcept {
            return m_context == other.m_context;
        }

    private:
        friend class io_context;
        explicit executor_type(io_context* ctx) noexcept : m_context(ctx) {
        }
        io_context* m_context;
    };

    explicit io_context(size_t t_concurrency_hint = 0);

    void run();
    size_t run_one();
    void stop() override;
    bool stopped() const noexcept override;
    void restart() override;
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

    execution::schedule_operation schedule() noexcept;

private:
    std::atomic<bool> m_stopped{false};
    execution::work_queue<> m_handlers;
    std::atomic<size_t> m_work_count{0};
    inline static thread_local io_context* current_context_ = nullptr;

    friend class execution::executor_work_guard;
    friend struct execution::schedule_operation;
};

}  // namespace svarog::io
