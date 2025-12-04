#pragma once

#include "svarog/network/detail/socket_types.hpp"
#include "svarog/network/ip/endpoint.hpp"

namespace svarog::network::ip {
class udp {
public:
    using endpoint = basic_endpoint<udp>;
    // using socket = basic_datagram_socket<udp>;

    constexpr udp() noexcept = default;

    static constexpr udp v4() noexcept {
        return udp{AF_INET};
    }

    static constexpr udp v6() noexcept {
        return udp{AF_INET6};
    }

    [[nodiscard]] constexpr int family() const noexcept {
        return m_family;
    }

    [[nodiscard]] static constexpr int type() noexcept {
        return SOCK_DGRAM;
    }

    [[nodiscard]] static constexpr int protocol() noexcept {
        return IPPROTO_UDP;
    }

    constexpr bool operator==(const udp&) const noexcept = default;

private:
    constexpr explicit udp(int t_family) noexcept : m_family(t_family) {
    }

    int m_family{AF_INET};
};
}  // namespace svarog::network::ip
