#pragma once

#include <system_error>
#include <utility>

#include "svarog/io/io_context.hpp"
#include "svarog/network/socket_base.hpp"

namespace svarog::network {

template <typename Protocol>
class basic_socket : public socket_base {
public:
    using protocol_type = Protocol;
    using endpoint_type = typename Protocol::endpoint;
    using native_handle_type = socket_base::native_handle_type;

    explicit basic_socket(io::io_context& t_ctx) : m_context(&t_ctx) {
    }

    basic_socket(io::io_context& t_ctx, const protocol_type& t_protocol) : m_context(&t_ctx) {
        open(t_protocol);
    }

    basic_socket(io::io_context& t_ctx, const endpoint_type& t_endpoint) : m_context(&t_ctx) {
        open(t_endpoint.protocol());
        bind(t_endpoint);
    }

    basic_socket(io::io_context& t_ctx, const protocol_type& t_protocol, native_handle_type t_native_socket)
        : m_context(&t_ctx), m_protocol(t_protocol), m_socket(t_native_socket) {
    }

    basic_socket(const basic_socket&) = delete;
    basic_socket& operator=(const basic_socket&) = delete;

    basic_socket(basic_socket&& t_other) noexcept
        : m_context(t_other.m_context),
          m_protocol(t_other.m_protocol),
          m_socket(std::exchange(t_other.m_socket, detail::invalid_socket)),
          m_non_blocking(t_other.m_non_blocking) {
    }

    basic_socket& operator=(basic_socket&& t_other) noexcept {
        if (this != &t_other) {
            close();
            m_context = t_other.m_context;
            m_protocol = t_other.m_protocol;
            m_socket = std::exchange(t_other.m_socket, detail::invalid_socket);
            m_non_blocking = t_other.m_non_blocking;
        }
        return *this;
    }

    ~basic_socket() {
        std::error_code ec;
        close(ec);
    }

    io::io_context& get_executor() noexcept {
        return *m_context;
    }

    void open(const protocol_type& t_protocol) {
        std::error_code ec;
        open(t_protocol, ec);
        if (ec) {
            throw std::system_error(ec, "open");
        }
    }

    void open(const protocol_type& t_protocol, std::error_code& t_ec) noexcept {
        if (is_open()) {
            t_ec = std::make_error_code(std::errc::already_connected);
            return;
        }

        m_socket = ::socket(t_protocol.family(), t_protocol.type(), t_protocol.protocol());
        if (m_socket == detail::invalid_socket) {
            t_ec = std::error_code(detail::last_socket_error(), std::system_category());
            return;
        }

        m_protocol = t_protocol;
        t_ec.clear();
    }

    void close() {
        std::error_code ec;
        close(ec);
        if (ec) {
            throw std::system_error(ec, "close");
        }
    }

    void close(std::error_code& t_ec) noexcept {
        if (!is_open()) {
            t_ec.clear();
            return;
        }

        if (detail::close_socket(m_socket) != 0) {
            t_ec = std::error_code(detail::last_socket_error(), std::system_category());
        } else {
            t_ec.clear();
        }
        m_socket = detail::invalid_socket;
    }

    [[nodiscard]] bool is_open() const noexcept {
        return m_socket != detail::invalid_socket;
    }

    [[nodiscard]] native_handle_type native_handle() const noexcept {
        return m_socket;
    }

    void assign(const protocol_type& t_protocol, native_handle_type t_native_socket) {
        std::error_code ec;
        assign(t_protocol, t_native_socket, ec);
        if (ec) {
            throw std::system_error(ec, "assign");
        }
    }

    void assign(const protocol_type& t_protocol, native_handle_type t_native_socket, std::error_code& t_ec) noexcept {
        if (is_open()) {
            t_ec = std::make_error_code(std::errc::already_connected);
            return;
        }
        m_protocol = t_protocol;
        m_socket = t_native_socket;
        t_ec.clear();
    }

    native_handle_type release() {
        return std::exchange(m_socket, detail::invalid_socket);
    }

    void bind(const endpoint_type& t_endpoint) {
        std::error_code ec;
        bind(t_endpoint, ec);
        if (ec) {
            throw std::system_error(ec, "bind");
        }
    }

    void bind(const endpoint_type& t_endpoint, std::error_code& t_ec) noexcept {
        if (::bind(m_socket, t_endpoint.data(), static_cast<socklen_t>(t_endpoint.size())) != 0) {
            t_ec = std::error_code(detail::last_socket_error(), std::system_category());
        } else {
            t_ec.clear();
        }
    }

    endpoint_type local_endpoint() const {
        std::error_code ec;
        auto ep = local_endpoint(ec);
        if (ec) {
            throw std::system_error(ec, "local_endpoint");
        }
        return ep;
    }

    endpoint_type local_endpoint(std::error_code& t_ec) const noexcept {
        endpoint_type ep;
        auto len = static_cast<socklen_t>(ep.capacity());
        if (::getsockname(m_socket, ep.data(), &len) != 0) {
            t_ec = std::error_code(detail::last_socket_error(), std::system_category());
        } else {
            ep.resize(len);
            t_ec.clear();
        }
        return ep;
    }

    void shutdown(shutdown_type t_what) {
        std::error_code ec;
        shutdown(t_what, ec);
        if (ec) {
            throw std::system_error(ec, "shutdown");
        }
    }

    void shutdown(shutdown_type t_what, std::error_code& t_ec) noexcept {
        if (::shutdown(m_socket, static_cast<int>(t_what)) != 0) {
            t_ec = std::error_code(detail::last_socket_error(), std::system_category());
        } else {
            t_ec.clear();
        }
    }

    template <typename Option>
    void set_option(const Option& t_option) {
        std::error_code ec;
        set_option(t_option, ec);
        if (ec) {
            throw std::system_error(ec, "set_option");
        }
    }

    template <typename Option>
    void set_option(const Option& t_option, std::error_code& t_ec) noexcept {
        if (::setsockopt(m_socket, Option::level(), Option::name(), reinterpret_cast<const char*>(t_option.data()),
                         static_cast<socklen_t>(t_option.size())) != 0) {
            t_ec = std::error_code(detail::last_socket_error(), std::system_category());
        } else {
            t_ec.clear();
        }
    }

    template <typename Option>
    void get_option(Option& t_option) const {
        std::error_code ec;
        get_option(t_option, ec);
        if (ec) {
            throw std::system_error(ec, "get_option");
        }
    }

    template <typename Option>
    void get_option(Option& t_option, std::error_code& t_ec) const noexcept {
        auto len = static_cast<socklen_t>(t_option.size());
        if (::getsockopt(m_socket, Option::level(), Option::name(), reinterpret_cast<char*>(t_option.data()), &len) !=
            0) {
            t_ec = std::error_code(detail::last_socket_error(), std::system_category());
        } else {
            t_option.resize(static_cast<std::size_t>(len));
            t_ec.clear();
        }
    }

    void non_blocking(bool t_mode) {
        std::error_code ec;
        non_blocking(t_mode, ec);
        if (ec) {
            throw std::system_error(ec, "non_blocking");
        }
    }

    void non_blocking(bool t_mode, std::error_code& t_ec) noexcept {
        if (detail::set_non_blocking(m_socket, t_mode) != 0) {
            t_ec = std::error_code(detail::last_socket_error(), std::system_category());
        } else {
            m_non_blocking = t_mode;
            t_ec.clear();
        }
    }

    [[nodiscard]] bool non_blocking() const {
        return m_non_blocking;
    }

    void wait(wait_type t_what) {
        std::error_code ec;
        wait(t_what, ec);
        if (ec) {
            throw std::system_error(ec, "wait");
        }
    }

    void wait(wait_type t_what, std::error_code& t_ec) {
        // Synchronous wait using poll/select
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(m_socket, &fds);

        fd_set* read_fds = (t_what == wait_type::read) ? &fds : nullptr;
        fd_set* write_fds = (t_what == wait_type::write) ? &fds : nullptr;
        fd_set* error_fds = (t_what == wait_type::error) ? &fds : nullptr;

#ifdef SVAROG_PLATFORM_WINDOWS
        int result = ::select(0, read_fds, write_fds, error_fds, nullptr);  // Windows ignores nfds
#else
        int result = ::select(m_socket + 1, read_fds, write_fds, error_fds, nullptr);
#endif
        if (result < 0) {
            t_ec = std::error_code(detail::last_socket_error(), std::system_category());
        } else {
            t_ec.clear();
        }
    }

    template <typename WaitHandler>
    void async_wait(wait_type t_what, WaitHandler&& t_handler) {
        auto& reactor = m_context->get_reactor();

        io::detail::io_operation ops = io::detail::io_operation::none;
        switch (t_what) {
        case wait_type::read:
            ops = io::detail::io_operation::read;
            break;
        case wait_type::write:
            ops = io::detail::io_operation::write;
            break;
        case wait_type::error:
            ops = io::detail::io_operation::error;
            break;
        }

        reactor.register_descriptor(
            m_socket, ops,
            [h = std::forward<WaitHandler>(t_handler)](std::error_code t_ec, std::size_t) mutable { h(t_ec); });
    }

protected:
    io::io_context* m_context{nullptr};
    protocol_type m_protocol{};
    native_handle_type m_socket{detail::invalid_socket};
    bool m_non_blocking{false};
};

}  // namespace svarog::network
