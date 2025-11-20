#pragma once

#include <functional>

#include "svarog/execution/awaitable_task.hpp"

#include <coroutine>

namespace svarog::io {
class io_context;
}

namespace svarog::execution {

struct schedule_operation {
    io::io_context* m_context;

    bool await_ready() const noexcept {
        return false;
    }

    void await_suspend(std::coroutine_handle<> handle) const;

    void await_resume() const noexcept {
    }
};

struct detached_t {};

inline constexpr detached_t detached{};

template <typename Awaitable>
    requires std::is_rvalue_reference_v<Awaitable&&>
void co_spawn(io::io_context& ctx, Awaitable&& awaitable, detached_t);

template <typename Awaitable, typename CompletionToken>
void co_spawn(io::io_context& ctx, Awaitable&& awaitable, CompletionToken&& token);

namespace this_coro {

extern thread_local io::io_context* current_context_;

inline io::io_context* current_executor() noexcept;

}  // namespace this_coro

}  // namespace svarog::execution
