#pragma once

#include <atomic>

namespace svarog::io {
class io_context;
}

namespace svarog::execution {

class executor_work_guard {
public:
    explicit executor_work_guard(io::io_context& ctx) noexcept;
    ~executor_work_guard();

    executor_work_guard(const executor_work_guard&) = delete;
    executor_work_guard& operator=(const executor_work_guard&) = delete;

    executor_work_guard(executor_work_guard&& other) noexcept;
    executor_work_guard& operator=(executor_work_guard&& other) noexcept;

    void reset() noexcept;

    bool owns_work() const noexcept {
        return m_context != nullptr;
    }

    io::io_context& get_executor() const noexcept;

private:
    io::io_context* m_context{nullptr};
};

inline executor_work_guard make_work_guard(io::io_context& ctx) noexcept {
    return executor_work_guard(ctx);
}

}  // namespace svarog::execution
