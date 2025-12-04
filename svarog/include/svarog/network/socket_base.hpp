#pragma once

#include "svarog/network/detail/socket_types.hpp"

namespace svarog::network {
class socket_base {
public:
    using native_handle_type = detail::native_socket_type;

    enum class shutdown_type {
        read = SHUT_RD,
        write = SHUT_WR,
        both = SHUT_RDWR
    };

    enum class wait_type {
        read,
        write,
        error
    };

    template <int Level, int Name>
    class boolean_option {
    public:
        constexpr boolean_option() noexcept = default;
        constexpr explicit boolean_option(bool t_value) noexcept : m_value(t_value), m_int_value(t_value ? 1 : 0) {
        }

        [[nodiscard]] constexpr bool value() const noexcept {
            return m_value;
        }

        constexpr explicit operator bool() const noexcept {
            return m_value;
        }

        static constexpr int level() noexcept {
            return Level;
        }

        static constexpr int name() noexcept {
            return Name;
        }

        int* data() noexcept {
            return &m_int_value;
        }

        [[nodiscard]] const int* data() const noexcept {
            return &m_int_value;
        }

        [[nodiscard]] constexpr std::size_t size() const noexcept {
            return sizeof(int);
        }

        // Called after getsockopt to update m_value from m_int_value
        void resize(std::size_t /*t_new_size*/) noexcept {
            m_value = (m_int_value != 0);
        }

    private:
        bool m_value{false};
        int m_int_value{};
    };

    template <int Level, int Name>
    class integer_option {
    public:
        constexpr integer_option() noexcept = default;
        constexpr explicit integer_option(int t_value) noexcept : m_value(t_value) {
        }

        [[nodiscard]] constexpr int value() const noexcept {
            return m_value;
        }

        static constexpr int level() noexcept {
            return Level;
        }

        static constexpr int name() noexcept {
            return Name;
        }

        int* data() noexcept {
            return &m_value;
        }

        [[nodiscard]] const int* data() const noexcept {
            return &m_value;
        }

        [[nodiscard]] constexpr std::size_t size() const noexcept {
            return sizeof(int);
        }

        void resize(std::size_t /*t_new_size*/) {
        }

    private:
        int m_value{0};
    };

    using reuse_address = boolean_option<SOL_SOCKET, SO_REUSEADDR>;
    using keep_alive = boolean_option<SOL_SOCKET, SO_KEEPALIVE>;
    using broadcast = boolean_option<SOL_SOCKET, SO_BROADCAST>;
    using debug = boolean_option<SOL_SOCKET, SO_DEBUG>;
    using do_not_route = boolean_option<SOL_SOCKET, SO_DONTROUTE>;

    using receive_buffer_size = integer_option<SOL_SOCKET, SO_RCVBUF>;
    using send_buffer_size = integer_option<SOL_SOCKET, SO_SNDBUF>;
    using receive_low_watermark = integer_option<SOL_SOCKET, SO_RCVLOWAT>;
    using send_low_watermark = integer_option<SOL_SOCKET, SO_SNDLOWAT>;

    class linger {
    public:
        linger() noexcept = default;
        linger(bool t_enable, int t_timeout) noexcept;

        [[nodiscard]] bool enabled() const noexcept;
        void enabled(bool t_value) noexcept;
        [[nodiscard]] int timeout() const noexcept;
        void timeout(int t_seconds) noexcept;

        static constexpr int level() noexcept {
            return SOL_SOCKET;
        }

        static constexpr int name() noexcept {
            return SO_LINGER;
        }

        ::linger* data() noexcept {
            return &m_value;
        }

        [[nodiscard]] const ::linger* data() const noexcept {
            return &m_value;
        }

        [[nodiscard]] static constexpr std::size_t size() noexcept {
            return sizeof(::linger);
        }

        void resize(std::size_t /*t_new_size*/) {
        }

    private:
        ::linger m_value{};
    };

protected:
    socket_base() = default;
    ~socket_base() = default;
};
}  // namespace svarog::network
