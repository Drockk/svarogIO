
#include "svarog/io/detail/kqueue_reactor.hpp"

#ifdef SVAROG_PLATFORM_MACOS

namespace svarog::io::detail {
kqueue_reactor::kqueue_reactor() : m_kqueue_fd(kqueue()) {
    if (m_kqueue_fd == -1) {
        throw std::system_error(errno, std::system_category(), "kqueue failed");
    }
}

void kqueue_reactor::do_register(native_handle_type t_fd, io_operation t_ops, completion_handler t_handler) {
    std::vector<struct kevent> changes;

    if (has_operation(t_ops, io_operation::read) || has_operation(t_ops, io_operation::accept)) {
        struct kevent ev;
        EV_SET(&ev, t_fd, EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, nullptr);
        changes.push_back(ev);
    }

    if (has_operation(t_ops, io_operation::write) || has_operation(t_ops, io_operation::connect)) {
        struct kevent ev;
        EV_SET(&ev, t_fd, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, nullptr);
        changes.push_back(ev);
    }

    {
        std::lock_guard lock(m_mutex);
        m_handlers[t_fd] = {std::move(t_handler), t_ops};
    }

    // Atomic registration
    if (kevent(m_kqueue_fd, changes.data(), changes.size(), nullptr, 0, nullptr) == -1) {
        std::lock_guard lock(m_mutex);
        m_handlers.erase(t_fd);
        throw std::system_error(errno, std::system_category(), "kevent registration failed");
    }
}

std::size_t kqueue_reactor::do_run_one(std::chrono::milliseconds t_timeout) {
    struct timespec ts;
    ts.tv_sec = t_timeout.count() / 1000;
    ts.tv_nsec = (t_timeout.count() % 1000) * 1000000;

    int n = kevent(m_kqueue_fd, nullptr, 0, m_events.data(), max_events, &ts);

    if (n == -1) {
        if (errno == EINTR)
            return 0;
        throw std::system_error(errno, std::system_category(), "kevent wait failed");
    }

    std::size_t processed = 0;
    for (int i = 0; i < n; ++i) {
        auto fd = static_cast<native_handle_type>(m_events[i].ident);

        completion_handler handler;
        {
            std::lock_guard lock(m_mutex);
            if (auto it = m_handlers.find(fd); it != m_handlers.end()) {
                handler = std::move(it->second.handler);
                m_handlers.erase(it);
            }
        }

        if (handler) {
            std::error_code ec;
            if (m_events[i].flags & EV_ERROR) {
                ec = std::error_code(static_cast<int>(m_events[i].data), std::system_category());
            }
            handler(ec, static_cast<std::size_t>(m_events[i].data));
            ++processed;
        }
    }
    return processed;
}
}  // namespace svarog::io::detail

#endif  // SVAROG_PLATFORM_MACOS
