#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <set>
#include <shared_mutex>
#include <unordered_map>

namespace svarog::io::detail {
using timer_id = std::uint64_t;
inline constexpr timer_id invalid_timer_id = 0;

using clock_type = std::chrono::steady_clock;
using time_point = clock_type::time_point;
using duration = clock_type::duration;

using timer_handler = std::move_only_function<void(std::error_code)>;

struct timer_entry {
    timer_id id;
    time_point deadline;
    mutable timer_handler handler;  // mutable: handler is consumed, not part of ordering key

    bool operator>(const timer_entry& t_other) const noexcept;
};

class timer_queue {
    using timer_set = std::set<timer_entry, std::greater<>>;

public:
    auto add_timer(time_point t_deadline, timer_handler t_handler) -> timer_id;
    auto add_timer(duration t_duration, timer_handler t_handler) -> timer_id;
    auto cancel_timer(timer_id t_id) -> bool;
    auto get_next_expiry() const -> std::optional<time_point>;
    auto time_until_next() const -> std::optional<duration>;
    auto pop_expired() -> std::optional<timer_handler>;
    auto has_expired(time_point t_now) const -> bool;
    auto process_expired(time_point t_now) -> size_t;
    auto empty() const noexcept -> bool;
    auto size() const noexcept -> size_t;
    void clear() noexcept;

private:
    auto generate_id() {
        return m_next_id.fetch_add(1, std::memory_order_relaxed);
    }

    std::atomic<timer_id> m_next_id{1};

    timer_set m_timers;
    std::unordered_map<timer_id, timer_set::iterator> m_id_to_iterator;

    mutable std::shared_mutex m_mutex;
};
}  // namespace svarog::io::detail
