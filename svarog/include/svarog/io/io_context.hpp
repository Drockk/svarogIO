#pragma once

#include <atomic>
#include <chrono>
#include <functional>

#include "svarog/core/contracts.hpp"
#include "svarog/execution/work_queue.hpp"
#include "svarog/io/detail/platform_config.hpp"

#if defined(SVAROG_PLATFORM_LINUX)
    #include "svarog/io/detail/epoll_reactor.hpp"
namespace svarog::io::detail {
using platform_reactor = epoll_reactor;
}
#elif defined(SVAROG_PLATFORM_MACOS)
    #include "svarog/io/detail/kqueue_reactor.hpp"
namespace svarog::io::detail {
using platform_reactor = kqueue_reactor;
}
#elif defined(SVAROG_PLATFORM_WINDOWS)
    #include "svarog/io/detail/iocp_reactor.hpp"
namespace svarog::io::detail {
using platform_reactor = iocp_reactor;
}
#else
    #include "svarog/io/detail/select_reactor.hpp"
namespace svarog::io::detail {
using platform_reactor = select_reactor;
}
#endif

namespace svarog::execution {
class executor_work_guard;
}  // namespace svarog::execution

namespace svarog::io {

class io_context {
public:
    class executor_type {
    public:
        void execute(std::move_only_function<void()> t_f) const;
        [[nodiscard]] io_context& context() const noexcept;

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

    detail::platform_reactor& get_reactor() noexcept {
        return m_reactor;
    }
    const detail::platform_reactor& get_reactor() const noexcept {
        return m_reactor;
    }

    void run();
    size_t run_one();

    size_t poll();

    size_t poll_one();

    void stop();
    [[nodiscard]] bool stopped() const noexcept;
    void restart();
    executor_type get_executor() noexcept;

    void post(auto&& t_handler) {
        [[maybe_unused]] bool pushed = m_handlers.push(std::forward<decltype(t_handler)>(t_handler));
        // TODO: Wake up reactor if blocked on epoll_wait/kevent
        // m_reactor.wake_up();
    }

    void dispatch(auto&& t_handler) {
        if (running_in_this_thread()) {
            SVAROG_EXPECTS(!stopped());
            t_handler();
        } else {
            post(std::forward<decltype(t_handler)>(t_handler));
        }
    }

    [[nodiscard]] bool running_in_this_thread() const noexcept;

private:
    [[nodiscard]] std::chrono::milliseconds calculate_timeout() const;

    [[nodiscard]] bool has_pending_work() const noexcept;

    // TODO: Implement when timer_queue is ready
    // void process_timers();
    // [[nodiscard]] std::chrono::milliseconds get_next_timer_timeout() const;

    std::atomic<bool> m_stopped{false};
    execution::work_queue<> m_handlers;
    std::atomic<size_t> m_work_count{0};
    inline static thread_local io_context* s_current_context = nullptr;

    detail::platform_reactor m_reactor;

    // TODO: Add when timer_queue is implemented
    // detail::timer_queue m_timer_queue;

    friend class execution::executor_work_guard;
};

}  // namespace svarog::io
