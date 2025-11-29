#include "svarog/io/detail//epoll_reactor.hpp"

#ifdef SVAROG_PLATFORM_LINUX

namespace svarog::io::detail {
epoll_reactor::epoll_reactor(trigger_mode t_mode) : m_epoll_fd(epoll_create1(EPOLL_CLOEXEC)), m_mode(t_mode) {
    if (m_epoll_fd == -1) {
        throw std::system_error(errno, std::system_category(), "epoll_create1 failed");
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

std::size_t epoll_reactor::do_run_one(std::chrono::milliseconds t_timeout) {
    auto timeout_ms = static_cast<int>(t_timeout.count());
    auto n = epoll_wait(m_epoll_fd, m_events.data(), max_events, timeout_ms);

    if (n == -1) {
        if (errno == EINTR)
            return 0;  // Interrupted, try again
        throw std::system_error(errno, std::system_category(), "epoll_wait failed");
    }

    std::size_t processed = 0;
    for (size_t i = 0; i < static_cast<size_t>(n); ++i) {
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
}  // namespace svarog::io::detail

#endif
