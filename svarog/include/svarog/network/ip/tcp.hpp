#pragma once

#include "svarog/network/detail/socket_types.hpp"
#include "svarog/network/ip/endpoint.hpp"

namespace svarog::network::ip {
class tcp {
public:
    using endpoint = basic_endpoint<tcp>;
    // using socket = basic_stream_socket<tcp>;
    // using acceptor = basic_acceptor<tcp>;

    static constexpr tcp v4() noexcept {
        return tcp{AF_INET};
    }

    static constexpr tcp v6() noexcept {
        return tcp{AF_INET6};
    }

    [[nodiscard]] constexpr int family() const noexcept {
        return m_family;
    }

    [[nodiscard]] static constexpr int type() noexcept {
        return SOCK_STREAM;
    }

    [[nodiscard]] static constexpr int protocol() noexcept {
        return IPPROTO_TCP;
    }

    constexpr bool operator==(const tcp&) const noexcept = default;

private:
    constexpr explicit tcp(int t_family) noexcept : m_family(t_family) {
    }

    int m_family{AF_INET};
};
}  // namespace svarog::network::ip
