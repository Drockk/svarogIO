#pragma once

#include <array>
#include <atomic>
#include <mutex>
#include <unordered_map>

#include "svarog/io/detail/platform_config.hpp"
#include "svarog/io/detail/reactor_interface.hpp"

#ifdef SVAROG_PLATFORM_LINUX

    #include <sys/epoll.h>
    #include <sys/socket.h>

namespace svarog::io::detail {
constexpr std::uint32_t to_epoll_events(io_operation t_ops) noexcept {
    std::uint32_t events = 0;
    if (has_operation(t_ops, io_operation::read))
        events |= EPOLLIN;
    if (has_operation(t_ops, io_operation::write))
        events |= EPOLLOUT;
    // EPOLLERR i EPOLLHUP są zawsze raportowane automatycznie
    return events;
}

constexpr io_operation from_epoll_events(std::uint32_t t_events) noexcept {
    io_operation ops = io_operation::none;
    if (t_events & EPOLLIN)
        ops = ops | io_operation::read;
    if (t_events & EPOLLOUT)
        ops = ops | io_operation::write;
    if (t_events & EPOLLERR)
        ops = ops | io_operation::error;
    if (t_events & EPOLLHUP)
        ops = ops | io_operation::hangup;
    return ops;
}

class epoll_reactor : public reactor_base<epoll_reactor> {
public:
    enum class trigger_mode : std::uint8_t {
        level_triggered,
        edge_triggered
    };

    explicit epoll_reactor(trigger_mode t_mode = trigger_mode::level_triggered);
    ~epoll_reactor();

    epoll_reactor(const epoll_reactor&) = delete;
    epoll_reactor& operator=(const epoll_reactor&) = delete;

private:
    friend class reactor_base<epoll_reactor>;

    void do_register(native_handle_type t_fd, io_operation t_ops, completion_handler t_handler);
    void do_unregister(native_handle_type t_fd);
    void do_modify(native_handle_type t_fd, io_operation t_ops);
    std::size_t do_run_one(std::chrono::milliseconds t_timeout);
    std::size_t do_poll_one();
    void do_stop();
    [[nodiscard]] bool is_stopped() const noexcept;

    int m_epoll_fd{-1};
    std::atomic<bool> m_stopped{false};
    trigger_mode m_mode;

    struct fd_data {
        completion_handler handler;
        io_operation operations;
    };
    std::unordered_map<native_handle_type, fd_data> m_handlers;
    mutable std::mutex m_mutex;

    // Event buffer dla epoll_wait
    static constexpr std::size_t max_events = 128;
    std::array<epoll_event, max_events> m_events{};
};
}  // namespace svarog::io::detail

#endif
