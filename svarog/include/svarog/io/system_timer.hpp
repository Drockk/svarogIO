#pragma once

#include <chrono>

#include "svarog/io/io_context.hpp"

namespace svarog::io {

class system_timer {
public:
    using clock_type = std::chrono::system_clock;
    using duration = clock_type::duration;
    using time_point = clock_type::time_point;

    static constexpr auto to_steady(time_point sys_tp) {
        auto sys_now = clock_type::now();
        auto steady_now = std::chrono::steady_clock::now();
        auto diff = sys_tp - sys_now;
        return steady_now + diff;
    }

    explicit system_timer(io_context& ctx)
        : m_context(ctx), m_expiry(time_point::max()), m_timer_id(detail::invalid_timer_id) {
    }

    system_timer(io_context& ctx, time_point t) : m_context(ctx), m_expiry(t), m_timer_id(detail::invalid_timer_id) {
    }

    system_timer(const system_timer&) = delete;
    system_timer& operator=(const system_timer&) = delete;

    system_timer(system_timer&& t_other) noexcept
        : m_context(t_other.m_context),
          m_expiry(t_other.m_expiry),
          m_timer_id(std::exchange(t_other.m_timer_id, detail::invalid_timer_id)) {
    }

    system_timer& operator=(system_timer&& t_other) noexcept {
        if (this != &t_other) {
            cancel();
            m_expiry = t_other.m_expiry;
            m_timer_id = std::exchange(t_other.m_timer_id, detail::invalid_timer_id);
        }
        return *this;
    }

    ~system_timer() {
        cancel();
    }

    void expires_at(time_point t) {
        cancel();
        m_expiry = t;
    }

    // Ustaw relative od teraz
    void expires_after(duration d) {
        cancel();
        m_expiry = clock_type::now() + d;
    }

    [[nodiscard]] time_point expiry() const noexcept {
        return m_expiry;
    }

    template <typename Handler>
    void async_wait(Handler&& handler) {
        cancel();

        // Convert to steady_clock for internal timer queue
        auto steady_expiry = to_steady(m_expiry);

        m_timer_id = m_context.get_timer_queue().add_timer(
            steady_expiry, [h = std::forward<Handler>(handler)](std::error_code ec) mutable { h(ec); });
    }

    std::size_t cancel() {
        if (m_timer_id == detail::invalid_timer_id)
            return 0;
        bool cancelled = m_context.get_timer_queue().cancel_timer(m_timer_id);
        m_timer_id = detail::invalid_timer_id;
        return cancelled ? 1 : 0;
    }

    [[nodiscard]] bool expired() const noexcept {
        return clock_type::now() >= m_expiry;
    }

private:
    io_context& m_context;
    time_point m_expiry;
    detail::timer_id m_timer_id;
};

}  // namespace svarog::io
