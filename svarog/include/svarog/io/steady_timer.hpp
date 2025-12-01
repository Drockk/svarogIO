#pragma once

#include <chrono>

#include "svarog/io/io_context.hpp"

namespace svarog::io {

class steady_timer {
public:
    using clock_type = std::chrono::steady_clock;
    using duration = clock_type::duration;
    using time_point = clock_type::time_point;

    struct durations {
        static constexpr duration zero = duration::zero();
        static constexpr duration max = duration::max();

        static constexpr auto milliseconds(std::int64_t t_ms) {
            return std::chrono::milliseconds(t_ms);
        }
        static constexpr auto seconds(std::int64_t t_s) {
            return std::chrono::seconds(t_s);
        }
        static constexpr auto minutes(std::int64_t t_m) {
            return std::chrono::minutes(t_m);
        }
    };

    explicit steady_timer(io_context& t_ctx)
        : m_context(t_ctx), m_expiry(time_point::max()), m_timer_id(detail::invalid_timer_id) {
    }

    steady_timer(io_context& t_ctx, duration t_d)
        : m_context(t_ctx), m_expiry(clock_type::now() + t_d), m_timer_id(detail::invalid_timer_id) {
    }

    steady_timer(io_context& t_ctx, time_point t_t)
        : m_context(t_ctx), m_expiry(t_t), m_timer_id(detail::invalid_timer_id) {
    }

    steady_timer(const steady_timer&) = delete;
    steady_timer& operator=(const steady_timer&) = delete;

    steady_timer(steady_timer&& t_other) noexcept
        : m_context(t_other.m_context),
          m_expiry(t_other.m_expiry),
          m_timer_id(std::exchange(t_other.m_timer_id, detail::invalid_timer_id)) {
    }

    steady_timer& operator=(steady_timer&& t_other) noexcept {
        if (this != &t_other) {
            cancel();
            // Note: m_context is a reference, cannot be reassigned
            // This operator only valid when both timers use same context
            m_expiry = t_other.m_expiry;
            m_timer_id = std::exchange(t_other.m_timer_id, detail::invalid_timer_id);
        }
        return *this;
    }

    ~steady_timer() {
        cancel();
    }

    void expires_after(duration t_d) {
        cancel();  // Cancel any pending wait
        m_expiry = clock_type::now() + t_d;
    }

    void expires_at(time_point t_t) {
        cancel();
        m_expiry = t_t;
    }

    [[nodiscard]] constexpr time_point expiry() const noexcept {
        return m_expiry;
    }

    // Handler signature: void(std::error_code)
    template <typename Handler>
    void async_wait(Handler&& t_handler) {
        cancel();  // Cancel previous wait if any

        m_timer_id = m_context.get_timer_queue().add_timer(
            m_expiry, [h = std::forward<Handler>(t_handler)](std::error_code ec) mutable { h(ec); });
    }

    // Cancel pending async operation
    // Returns number of cancelled operations (0 or 1)
    std::size_t cancel() {
        if (m_timer_id == detail::invalid_timer_id) {
            return 0;
        }

        bool cancelled = m_context.get_timer_queue().cancel_timer(m_timer_id);
        m_timer_id = detail::invalid_timer_id;

        return cancelled ? 1 : 0;
    }

    // Check if timer expired
    [[nodiscard]] bool expired() const noexcept {
        return clock_type::now() >= m_expiry;
    }

private:
    io_context& m_context;
    time_point m_expiry;
    detail::timer_id m_timer_id;
};

}  // namespace svarog::io
