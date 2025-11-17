#include "svarog/execution/work_queue.hpp"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <utility>

#include "svarog/core/contracts.hpp"

class work_queue_impl {
public:
    bool push(svarog::execution::work_item&& t_item) {
        const std::lock_guard guard(m_mutex);

        if (m_stopped.load()) {
            return false;
        }

        m_queue.push_back(std::move(t_item));
        m_cv.notify_one();

        return true;
    }

    svarog::execution::expected<svarog::execution::work_item, svarog::execution::queue_error> pop() noexcept {
        std::unique_lock lock(m_mutex);

        m_cv.wait(lock, [this] { return !m_queue.empty() || m_stopped.load(); });

        if (m_stopped.load()) {
            return svarog::execution::unexpected(svarog::execution::queue_error::stopped);
        }

        auto item = std::move(m_queue.front());
        m_queue.pop_front();
        return item;
    }

    svarog::execution::expected<svarog::execution::work_item, svarog::execution::queue_error> try_pop() noexcept {
        const std::lock_guard guard(m_mutex);

        if (m_queue.empty()) {
            return svarog::execution::unexpected(m_stopped.load() ? svarog::execution::queue_error::stopped
                                                                  : svarog::execution::queue_error::empty);
        }

        auto item = std::move(m_queue.front());
        m_queue.pop_front();
        return item;
    }

    size_t size() const noexcept {
        const std::lock_guard guard(m_mutex);
        return m_queue.size();
    }

    bool empty() const noexcept {
        const std::lock_guard guard(m_mutex);
        return m_queue.empty();
    }

    void stop() noexcept {
        m_stopped.store(true);
        m_cv.notify_all();
    }

    bool stopped() const noexcept {
        return m_stopped.load();
    }

    void clear() noexcept {
        m_queue.clear();
    }

private:
    std::atomic<bool> m_stopped{false};
    std::condition_variable m_cv;
    std::deque<svarog::execution::work_item> m_queue;
    mutable std::mutex m_mutex;
};

namespace svarog::execution {
work_queue::work_queue() : m_impl(std::make_unique<work_queue_impl>()) {
}

work_queue::~work_queue() = default;

bool work_queue::push(work_item&& t_item) {
    SVAROG_EXPECTS(t_item != nullptr);
    return m_impl->push(std::move(t_item));
}

expected<work_item, queue_error> work_queue::pop() noexcept {
    return m_impl->pop();
}

expected<work_item, queue_error> work_queue::try_pop() noexcept {
    return m_impl->try_pop();
}

size_t work_queue::size() const noexcept {
    return m_impl->size();
}

bool work_queue::empty() const noexcept {
    return m_impl->empty();
}

void work_queue::stop() noexcept {
    m_impl->stop();
}

bool work_queue::stopped() const noexcept {
    return m_impl->stopped();
}

void work_queue::clear() noexcept {
    m_impl->clear();
}

}  // namespace svarog::execution
