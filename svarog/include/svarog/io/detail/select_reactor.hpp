#include <array>
#include <atomic>
#include <vector>

#include <poll.h>

#include "svarog/io/detail/reactor_interface.hpp"

namespace svarog::io::detail {

class select_reactor : public reactor_base<select_reactor> {
public:
    select_reactor() = default;
    ~select_reactor() = default;

private:
    friend class reactor_base<select_reactor>;

    void do_register(native_handle_type t_fd, io_operation t_ops, completion_handler t_handler);
    void do_unregister(native_handle_type t_fd);
    void do_modify(native_handle_type t_fd, io_operation t_ops);
    std::size_t do_run_one(std::chrono::milliseconds t_timeout);
    std::size_t do_poll_one();
    void do_stop();
    [[nodiscard]] bool is_stopped() const noexcept;

    std::atomic<bool> m_stopped{false};

    struct fd_entry {
        native_handle_type fd;
        io_operation operations;
        completion_handler handler;
    };
    std::vector<fd_entry> m_descriptors;
    mutable std::mutex m_mutex;

#ifndef _WIN32
    std::array<int, 2> m_wake_pipe{-1, -1};
#else

#endif
};

// Implementacja z użyciem poll()
std::size_t select_reactor::do_run_one(std::chrono::milliseconds t_timeout) {
    std::vector<pollfd> pollfds;
    std::vector<fd_entry> snapshot;

    {
        std::lock_guard lock(m_mutex);
        snapshot = m_descriptors;
    }

    // Dodaj wake pipe
#ifndef _WIN32
    pollfds.push_back({m_wake_pipe[0], POLLIN, 0});
#endif

    for (const auto& entry : snapshot) {
        pollfd pfd{};
        pfd.fd = entry.fd;
        if (has_operation(entry.operations, io_operation::read))
            pfd.events |= POLLIN;
        if (has_operation(entry.operations, io_operation::write))
            pfd.events |= POLLOUT;
        pollfds.push_back(pfd);
    }

    int result = poll(pollfds.data(), pollfds.size(), static_cast<int>(t_timeout.count()));

    if (result == -1) {
        if (errno == EINTR)
            return 0;
        throw std::system_error(errno, std::system_category(), "poll failed");
    }

    if (result == 0)
        return 0;  // Timeout

    std::size_t processed = 0;

    // Check wake pipe first
#ifndef _WIN32
    if (pollfds[0].revents & POLLIN) {
        char buf{};
        read(m_wake_pipe[0], &buf, 1);
        return 0;  // Woken up for stop
    }
#endif

    for (std::size_t i = 1; i < pollfds.size(); ++i) {
        if (pollfds[i].revents == 0)
            continue;

        auto& entry = snapshot[i - 1];

        // Find and remove handler
        completion_handler handler;
        {
            std::lock_guard lock(m_mutex);
            auto it = std::find_if(m_descriptors.begin(), m_descriptors.end(),
                                   [fd = entry.fd](const fd_entry& e) { return e.fd == fd; });
            if (it != m_descriptors.end()) {
                handler = std::move(it->handler);
                m_descriptors.erase(it);
            }
        }

        if (handler) {
            std::error_code ec;
            if (pollfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                ec = std::make_error_code(std::errc::io_error);
            }
            handler(ec, 0);
            ++processed;
        }
    }

    return processed;
}

}  // namespace svarog::io::detail
