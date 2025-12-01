#include "svarog/io/detail/epoll_reactor.hpp"

#ifdef SVAROG_PLATFORM_LINUX

    #include <unistd.h>

namespace svarog::io::detail {

epoll_reactor::epoll_reactor(trigger_mode t_mode) : m_epoll_fd(epoll_create1(EPOLL_CLOEXEC)), m_mode(t_mode) {
    if (m_epoll_fd == -1) {
        throw std::system_error(errno, std::system_category(), "epoll_create1 failed");
    }
}

epoll_reactor::~epoll_reactor() {
    if (m_epoll_fd != -1) {
        ::close(m_epoll_fd);
        m_epoll_fd = -1;
    }
}

void epoll_reactor::do_register(native_handle_type t_fd, io_operation t_ops, completion_handler t_handler) {
    epoll_event ev{};
    ev.events = to_epoll_events(t_ops);
    if (m_mode == trigger_mode::edge_triggered) {
        ev.events |= EPOLLET;
    }
    ev.data.fd = t_fd;

    {
        std::lock_guard lock(m_mutex);
        m_handlers[t_fd] = {std::move(t_handler), t_ops};
    }

    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, t_fd, &ev) == -1) {
        std::lock_guard lock(m_mutex);
        m_handlers.erase(t_fd);
        throw std::system_error(errno, std::system_category(), "epoll_ctl ADD failed");
    }
}

void epoll_reactor::do_unregister(native_handle_type t_fd) {
    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, t_fd, nullptr) == -1) {
        // Ignore ENOENT - fd may have been closed already
        if (errno != ENOENT) {
            throw std::system_error(errno, std::system_category(), "epoll_ctl DEL failed");
        }
    }

    std::lock_guard lock(m_mutex);
    m_handlers.erase(t_fd);
}

void epoll_reactor::do_modify(native_handle_type t_fd, io_operation t_ops) {
    epoll_event ev{};
    ev.events = to_epoll_events(t_ops);
    if (m_mode == trigger_mode::edge_triggered) {
        ev.events |= EPOLLET;
    }
    ev.data.fd = t_fd;

    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, t_fd, &ev) == -1) {
        throw std::system_error(errno, std::system_category(), "epoll_ctl MOD failed");
    }

    std::lock_guard lock(m_mutex);
    if (auto it = m_handlers.find(t_fd); it != m_handlers.end()) {
        it->second.operations = t_ops;
    }
}

std::size_t epoll_reactor::do_run_one(std::chrono::milliseconds t_timeout) {
    if (m_stopped.load(std::memory_order_acquire)) {
        return 0;
    }

    auto timeout_ms = static_cast<int>(t_timeout.count());
    auto n = epoll_wait(m_epoll_fd, m_events.data(), max_events, timeout_ms);

    if (n == -1) {
        if (errno == EINTR)
            return 0;  // Interrupted, try again
        throw std::system_error(errno, std::system_category(), "epoll_wait failed");
    }

    std::size_t processed = 0;
    for (std::size_t i = 0; i < static_cast<std::size_t>(n); ++i) {
        auto fd = m_events.at(i).data.fd;
        auto events = from_epoll_events(m_events.at(i).events);

        completion_handler handler;
        {
            std::lock_guard lock(m_mutex);
            if (auto it = m_handlers.find(fd); it != m_handlers.end()) {
                handler = std::move(it->second.handler);
                m_handlers.erase(it);  // One-shot semantics
            }
        }

        if (handler) {
            std::error_code ec;
            if (has_operation(events, io_operation::error)) {
                int error = 0;
                socklen_t len = sizeof(error);
                getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len);
                ec = std::error_code(error, std::system_category());
            }
            handler(ec, 0);  // bytes_transferred filled by actual I/O op
            ++processed;
        }
    }
    return processed;
}

std::size_t epoll_reactor::do_poll_one() {
    return do_run_one(std::chrono::milliseconds::zero());
}

void epoll_reactor::do_stop() {
    m_stopped.store(true, std::memory_order_release);
}

bool epoll_reactor::is_stopped() const noexcept {
    return m_stopped.load(std::memory_order_acquire);
}

}  // namespace svarog::io::detail

#endif  // SVAROG_PLATFORM_LINUX
