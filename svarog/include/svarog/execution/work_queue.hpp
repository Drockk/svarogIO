#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <list>
#include <mutex>
#include <queue>
#include <vector>

#include "svarog/core/contracts.hpp"

// Use std::expected if available (C++23), otherwise use tl::expected (backport)
#if __cpp_lib_expected >= 202202L
    #include <expected>  // std::expected, std::unexpected
namespace svarog::execution {
template <typename T, typename E>
using expected = std::expected<T, E>;
using std::unexpected;
}  // namespace svarog::execution
#else
    #include <tl/expected.hpp>  // tl::expected (backport for pre-C++23)
namespace svarog::execution {
template <typename T, typename E>
using expected = tl::expected<T, E>;
using tl::unexpected;
}  // namespace svarog::execution
#endif

namespace svarog::execution {

using work_item = std::move_only_function<void()>;

template <typename Container>
concept WorkQueueContainer = requires(Container t_container) {
    typename Container::value_type;
    { t_container.push_back(std::declval<typename Container::value_type>()) } -> std::same_as<void>;
    { t_container.front() } -> std::same_as<typename Container::value_type&>;
    { t_container.pop_front() } -> std::same_as<void>;
    { t_container.empty() } -> std::convertible_to<bool>;
    { t_container.size() } -> std::convertible_to<std::size_t>;
};

static_assert(WorkQueueContainer<std::deque<work_item>>);
static_assert(WorkQueueContainer<std::list<work_item>>);
static_assert(!WorkQueueContainer<std::queue<work_item>>);
static_assert(!WorkQueueContainer<std::vector<work_item>>);

enum class queue_error : std::uint8_t {
    empty,
    stopped
};

template <WorkQueueContainer Container = std::deque<work_item>>
class work_queue {
public:
    work_queue() = default;

    ~work_queue() {
        stop();
    }

    work_queue(const work_queue&) = delete;
    work_queue& operator=(const work_queue&) = delete;
    work_queue(work_queue&&) = delete;
    work_queue& operator=(work_queue&&) = delete;

    [[nodiscard]] bool push(work_item&& t_item) {
        SVAROG_EXPECTS(t_item != nullptr);

        const std::lock_guard guard(m_mutex);

        if (m_stopped.load()) {
            return false;
        }

        m_queue.push_back(std::move(t_item));
        m_cv.notify_one();

        return true;
    }

    [[nodiscard]] expected<work_item, queue_error> pop() noexcept {
        std::unique_lock lock(m_mutex);

        m_cv.wait(lock, [this] { return !m_queue.empty() || m_stopped.load(); });

        if (m_stopped.load()) {
            return unexpected{queue_error::stopped};
        }

        auto item = std::move(m_queue.front());
        m_queue.pop_front();
        return item;
    }

    [[nodiscard]] expected<work_item, queue_error> pop(std::move_only_function<bool()> t_stop_predicate) noexcept {
        std::unique_lock lock(m_mutex);

        m_cv.wait(lock,
                  [this, &t_stop_predicate] { return !m_queue.empty() || m_stopped.load() || t_stop_predicate(); });

        if (m_stopped.load()) {
            return unexpected{queue_error::stopped};
        }

        if (m_queue.empty()) {
            return unexpected{queue_error::empty};
        }

        auto item = std::move(m_queue.front());
        m_queue.pop_front();
        return item;
    }

    [[nodiscard]] expected<work_item, queue_error> try_pop() noexcept {
        const std::lock_guard guard(m_mutex);

        if (m_queue.empty()) {
            return unexpected{m_stopped.load() ? queue_error::stopped : queue_error::empty};
        }

        auto item = std::move(m_queue.front());
        m_queue.pop_front();
        return item;
    }

    [[nodiscard]] size_t size() const noexcept {
        std::lock_guard _(m_mutex);
        return m_queue.size();
    }

    [[nodiscard]] bool empty() const noexcept {
        std::lock_guard _(m_mutex);
        return m_queue.empty();
    }

    void stop() noexcept {
        m_stopped.store(true);
        m_cv.notify_all();
    }

    [[nodiscard]] bool stopped() const noexcept {
        return m_stopped.load();
    }

    void clear() noexcept {
        std::lock_guard _(m_mutex);
        m_queue.clear();
    }

    void notify_all() noexcept {
        m_cv.notify_all();
    }

private:
    std::atomic<bool> m_stopped{false};
    std::condition_variable m_cv;
    Container m_queue;
    mutable std::mutex m_mutex;
};

}  // namespace svarog::execution
