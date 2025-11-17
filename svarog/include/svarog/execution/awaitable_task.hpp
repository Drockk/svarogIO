#pragma once

#include <exception>
#include <optional>
#include <utility>

#include <coroutine>

namespace svarog::execution {

template <typename T = void>
class awaitable_task;

namespace detail {

template <typename T>
struct awaitable_task_promise_base {
    std::suspend_always initial_suspend() noexcept {
        return {};
    }

    struct final_awaiter {
        bool await_ready() noexcept {
            return false;
        }

        template <typename Promise>
        std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise> h) noexcept {
            auto& promise = h.promise();
            if (promise.m_continuation) {
                return promise.m_continuation;
            }
            return std::noop_coroutine();
        }

        void await_resume() noexcept {
        }
    };

    auto final_suspend() noexcept -> final_awaiter {
        return final_awaiter{};
    }

    void unhandled_exception() noexcept {
        m_exception = std::current_exception();
    }

    std::coroutine_handle<> m_continuation{};
    std::exception_ptr m_exception{};
};

template <typename T>
struct awaitable_task_promise : awaitable_task_promise_base<T> {
    awaitable_task<T> get_return_object() noexcept;

    void return_value(T&& value) noexcept {
        m_result.emplace(std::forward<T>(value));
    }

    void return_value(const T& value) noexcept {
        m_result.emplace(value);
    }

    T result() {
        if (this->m_exception) {
            std::rethrow_exception(this->m_exception);
        }
        return std::move(*m_result);
    }

    std::optional<T> m_result;
};

template <>
struct awaitable_task_promise<void> : awaitable_task_promise_base<void> {
    awaitable_task<void> get_return_object() noexcept;

    void return_void() noexcept {
    }

    void result() {
        if (this->m_exception) {
            std::rethrow_exception(this->m_exception);
        }
    }
};

}  // namespace detail

template <typename T>
class awaitable_task {
public:
    using promise_type = detail::awaitable_task_promise<T>;
    using handle_type = std::coroutine_handle<promise_type>;

    awaitable_task() noexcept = default;

    explicit awaitable_task(handle_type handle) noexcept : m_handle(handle) {
    }

    awaitable_task(awaitable_task&& other) noexcept : m_handle(std::exchange(other.m_handle, {})) {
    }

    awaitable_task& operator=(awaitable_task&& other) noexcept {
        if (this != &other) {
            if (m_handle) {
                m_handle.destroy();
            }
            m_handle = std::exchange(other.m_handle, {});
        }
        return *this;
    }

    ~awaitable_task() {
        if (m_handle) {
            m_handle.destroy();
        }
    }

    awaitable_task(const awaitable_task&) = delete;
    awaitable_task& operator=(const awaitable_task&) = delete;

    bool await_ready() const noexcept {
        return !m_handle || m_handle.done();
    }

    auto await_suspend(auto awaiting) noexcept -> std::coroutine_handle<> {
        m_handle.promise().m_continuation = awaiting;
        return m_handle;
    }

    T await_resume() {
        return m_handle.promise().result();
    }

    handle_type handle() const noexcept {
        return m_handle;
    }

    bool valid() const noexcept {
        return m_handle != nullptr;
    }

private:
    handle_type m_handle{};
};

namespace detail {

template <typename T>
awaitable_task<T> awaitable_task_promise<T>::get_return_object() noexcept {
    return awaitable_task<T>{std::coroutine_handle<awaitable_task_promise<T>>::from_promise(*this)};
}

inline awaitable_task<void> awaitable_task_promise<void>::get_return_object() noexcept {
    return awaitable_task<void>{std::coroutine_handle<awaitable_task_promise<void>>::from_promise(*this)};
}

}  // namespace detail

}  // namespace svarog::execution
