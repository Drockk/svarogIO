#include "svarog/execution/co_spawn.hpp"

#include "svarog/io/io_context.hpp"

namespace svarog::execution {

void schedule_operation::await_suspend(std::coroutine_handle<> handle) {
    m_context->post([handle]() mutable { handle.resume(); });
}

namespace detail {

template <typename Awaitable>
awaitable_task<void> spawn_impl(Awaitable awaitable) {
    try {
        co_await std::move(awaitable);
    } catch (...) {
        // Absorb exceptions in detached mode
    }
}

}  // namespace detail

template <typename Awaitable>
    requires std::is_rvalue_reference_v<Awaitable&&>
void co_spawn(io::io_context& ctx, Awaitable&& awaitable, detached_t) {
    auto task_copy = std::forward<Awaitable>(awaitable);

    ctx.post([task = std::move(task_copy)]() mutable {
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

        auto launcher = [](auto t) -> detached_coroutine { co_await std::move(t); };

        launcher(std::move(task));
    });
}

// Explicit instantiations
template void co_spawn(io::io_context&, awaitable_task<void>&&, detached_t);
template void co_spawn(io::io_context&, awaitable_task<int>&&, detached_t);

namespace this_coro {

thread_local io::io_context* current_context_ = nullptr;

io::io_context* current_executor() noexcept {
    return current_context_;
}

}  // namespace this_coro

}  // namespace svarog::execution
