#include <atomic>

#include "svarog/io/detail/platform_config.hpp"
#include "svarog/io/detail/reactor_interface.hpp"

#ifdef SVAROG_PLATFORM_MACOS

    #include <sys/event.h>

namespace svarog::io::detail {

// constexpr mapowanie io_operation -> kqueue filters
constexpr std::int16_t to_kqueue_filter(io_operation t_op) noexcept {
    if (has_operation(t_op, io_operation::read) || has_operation(t_op, io_operation::accept)) {
        return EVFILT_READ;
    }
    if (has_operation(t_op, io_operation::write) || has_operation(t_op, io_operation::connect)) {
        return EVFILT_WRITE;
    }
    return 0;
}

// constexpr flagi kqueue
struct kqueue_flags {
    static constexpr std::uint16_t add = EV_ADD;
    static constexpr std::uint16_t delete_ = EV_DELETE;
    static constexpr std::uint16_t enable = EV_ENABLE;
    static constexpr std::uint16_t disable = EV_DISABLE;
    static constexpr std::uint16_t oneshot = EV_ONESHOT;  // Auto-remove after trigger
    static constexpr std::uint16_t clear = EV_CLEAR;      // Reset state after delivery
    static constexpr std::uint16_t eof = EV_EOF;
    static constexpr std::uint16_t error = EV_ERROR;
};

class kqueue_reactor : public reactor_base<kqueue_reactor> {
public:
    kqueue_reactor();
    ~kqueue_reactor();

    kqueue_reactor(const kqueue_reactor&) = delete;
    kqueue_reactor& operator=(const kqueue_reactor&) = delete;

private:
    friend class reactor_base<kqueue_reactor>;

    void do_register(native_handle_type t_fd, io_operation t_ops, completion_handler t_handler);
    void do_unregister(native_handle_type t_fd);
    void do_modify(native_handle_type t_fd, io_operation t_ops);
    std::size_t do_run_one(std::chrono::milliseconds t_timeout);
    std::size_t do_poll_one();
    void do_stop();
    [[nodiscard]] bool is_stopped() const noexcept;

    int m_kqueue_fd{-1};
    std::atomic<bool> m_stopped{false};

    struct fd_data {
        completion_handler handler;
        io_operation operations;
    };
    std::unordered_map<native_handle_type, fd_data> m_handlers{};
    mutable std::mutex m_mutex;

    static constexpr std::size_t max_events = 128;
    std::array<struct kevent, max_events> m_events{};
};

}  // namespace svarog::io::detail

#endif  // SVAROG_PLATFORM_MACOS
