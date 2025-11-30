#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <system_error>

namespace svarog::io::detail {
enum class io_operation : std::uint8_t {
    none = 0,
    read = 1 << 0,     // 0b0001
    write = 1 << 1,    // 0b0010
    accept = 1 << 2,   // 0b0100
    connect = 1 << 3,  // 0b1000
    error = 1 << 4,    // 0b10000
    hangup = 1 << 5    // 0b100000
};

constexpr io_operation operator|(io_operation t_a, io_operation t_b) noexcept {
    return static_cast<io_operation>(static_cast<std::uint8_t>(t_a) | static_cast<std::uint8_t>(t_b));
}

constexpr io_operation operator&(io_operation t_a, io_operation t_b) noexcept {
    return static_cast<io_operation>(static_cast<std::uint8_t>(t_a) & static_cast<std::uint8_t>(t_b));
}

constexpr bool has_operation(io_operation t_mask, io_operation t_op) noexcept {
    return (t_mask & t_op) == t_op;
}

#if defined(_WIN32)
using native_handle_type = SOCKET;
inline constexpr native_handle_type invalid_handle = INVALID_SOCKET;
#else
using native_handle_type = int;
inline constexpr native_handle_type invalid_handle = -1;
#endif

using completion_handler = std::move_only_function<void(std::error_code, std::size_t)>;

template <typename Derived>
class reactor_base {
public:
    void register_descriptor(native_handle_type t_fd, io_operation t_ops, completion_handler t_handler) {
        static_cast<Derived*>(this)->do_register(t_fd, t_ops, std::move(t_handler));
    }

    void unregister_descriptor(native_handle_type t_fd) {
        static_cast<Derived*>(this)->do_unregister(t_fd);
    }

    void modify_descriptor(native_handle_type t_fd, io_operation t_ops) {
        static_cast<Derived*>(this)->do_modify(t_fd, t_ops);
    }

    std::size_t run_one(std::chrono::milliseconds t_timeout) {
        return static_cast<Derived*>(this)->do_run_one(t_timeout);
    }

    std::size_t poll_one() {
        return static_cast<Derived*>(this)->do_poll_one();
    }

    void stop() {
        static_cast<Derived*>(this)->do_stop();
    }

    [[nodiscard]] bool stopped() const noexcept {
        return static_cast<const Derived*>(this)->is_stopped();
    }
};
}  // namespace svarog::io::detail
