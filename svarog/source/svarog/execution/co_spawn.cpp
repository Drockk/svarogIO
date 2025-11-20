#include "svarog/execution/co_spawn.hpp"

#include "svarog/execution/awaitable_task.hpp"
#include "svarog/io/io_context.hpp"

#include <coroutine>

namespace svarog::execution {

void schedule_operation::await_suspend(std::coroutine_handle<> t_handle) const {
    m_context->post([t_handle]() mutable { t_handle.resume(); });
}

namespace detail {

template <typename Awaitable>
awaitable_task<void> spawn_impl(Awaitable t_awaitable) {
    try {
        co_await std::move(t_awaitable);
    } catch (...) {  // NOLINT(bugprone-empty-catch) - Intentional exception absorption in detached mode
        // Absorb exceptions in detached mode
    }
}

}  // namespace detail

template <typename Awaitable>
    requires std::is_rvalue_reference_v<Awaitable&&>
void co_spawn(io::io_context& t_ctx, Awaitable&& t_awaitable, detached_t /*t_detached*/) {
    auto task_copy = std::forward<Awaitable>(t_awaitable);

    t_ctx.post([task = std::move(task_copy)]() mutable {
        struct detached_coroutine {
            struct promise_type {
                detached_coroutine get_return_object() {
                    return {};
                }
                std::suspend_never initial_suspend() {
                    return {};
                }
                std::suspend_never final_suspend() noexcept {
                    return {};
                }
                void return_void() {
                }
                void unhandled_exception() {
                }
            };
        };

        auto launcher = [](auto t_task) -> detached_coroutine { co_await std::move(t_task); };

        launcher(std::move(task));
    });
}

// Explicit instantiations
template void co_spawn(io::io_context&, awaitable_task<void>&&, detached_t);
template void co_spawn(io::io_context&, awaitable_task<int>&&, detached_t);

namespace this_coro {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables) - Thread-local state is acceptable pattern
thread_local io::io_context* current_context_ = nullptr;

io::io_context* current_executor() noexcept {
    return current_context_;
}

}  // namespace this_coro

}  // namespace svarog::execution
