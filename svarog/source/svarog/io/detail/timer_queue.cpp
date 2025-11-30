#include "svarog/io/detail/timer_queue.hpp"

#include <mutex>

namespace svarog::io::detail {
bool timer_entry::operator>(const timer_entry& t_other) const noexcept {
    return deadline > t_other.deadline;
}

auto timer_queue::add_timer(time_point t_deadline, timer_handler t_handler) -> timer_id {
    std::unique_lock lock(m_mutex);
    const auto id = generate_id();

    const auto [it, inserted] = m_timers.emplace(timer_entry{id, t_deadline, std::move(t_handler)});
    if (inserted) {
        m_id_to_iterator[id] = it;
    }

    return id;
}

auto timer_queue::add_timer(duration t_duration, timer_handler t_handler) -> timer_id {
    return add_timer(clock_type::now() + t_duration, std::move(t_handler));
}

auto timer_queue::cancel_timer(timer_id t_id) -> bool {
    std::unique_lock lock(m_mutex);

    const auto it = m_id_to_iterator.find(t_id);
    if (it == m_id_to_iterator.end()) {
        return false;
    }

    m_timers.erase(it->second);
    m_id_to_iterator.erase(it);
    return true;
}

auto timer_queue::get_next_expiry() const -> std::optional<time_point> {
    std::shared_lock lock(m_mutex);
    if (m_timers.empty()) {
        return std::nullopt;
    }
    return m_timers.begin()->deadline;
}

auto timer_queue::time_until_next() const -> std::optional<duration> {
    auto expiry = get_next_expiry();
    if (!expiry) {
        return std::nullopt;
    }
    auto now = clock_type::now();
    if (*expiry <= now) {
        return duration::zero();
    }
    return *expiry - now;
}

auto timer_queue::pop_expired() -> std::optional<timer_handler> {
    std::unique_lock lock(m_mutex);
    if (m_timers.empty()) {
        return std::nullopt;
    }
    auto it = m_timers.begin();
    if (it->deadline > clock_type::now()) {
        return std::nullopt;
    }
    auto handler = std::move(it->handler);
    m_id_to_iterator.erase(it->id);
    m_timers.erase(it);
    return handler;
}

auto timer_queue::has_expired(time_point t_now) const -> bool {
    std::shared_lock lock(m_mutex);
    if (m_timers.empty()) {
        return false;
    }
    return m_timers.begin()->deadline <= t_now;
}

auto timer_queue::process_expired(time_point t_now) -> size_t {
    size_t count = 0;
    while (auto handler = pop_expired()) {
        if (clock_type::now() > t_now) {
            break;
        }
        (*handler)(std::error_code{});
        ++count;
    }
    return count;
}

auto timer_queue::empty() const noexcept -> bool {
    std::shared_lock lock(m_mutex);
    return m_timers.empty();
}

auto timer_queue::size() const noexcept -> size_t {
    std::shared_lock lock(m_mutex);
    return m_timers.size();
}

void timer_queue::clear() noexcept {
    std::unique_lock lock(m_mutex);
    for (const auto& entry : m_timers) {
        if (entry.handler) {
            std::move(entry.handler)(std::make_error_code(std::errc::operation_canceled));
        }
    }
    m_timers.clear();
    m_id_to_iterator.clear();
}
}  // namespace svarog::io::detail
