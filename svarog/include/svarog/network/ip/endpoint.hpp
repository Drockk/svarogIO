#pragma once

#include <cstring>

#include "svarog/network/ip/address.hpp"
#include "svarog/network/ip/address_v4.hpp"
#include "svarog/network/ip/address_v6.hpp"

namespace svarog::network::ip {
template <typename Protocol>
class basic_endpoint {
public:
    using protocol_type = Protocol;

    constexpr basic_endpoint() noexcept : m_data{} {
        m_data.m_v4.sin_family = AF_INET;
        m_data.m_v4.sin_port = 0;
        m_data.m_v4.sin_addr.s_addr = INADDR_ANY;
    }

    basic_endpoint(const ip::address& t_address, std::uint16_t t_port) noexcept {
        if (t_address.is_v4()) {
            m_data.m_v4.sin_family = AF_INET;
            m_data.m_v4.sin_port = htons(t_port);
            m_data.m_v4.sin_addr = t_address.to_v4().to_native();
        } else {
            m_data.m_v6.sin6_family = AF_INET6;
            m_data.m_v6.sin6_port = htons(t_port);
            m_data.m_v6.sin6_flowinfo = 0;
            m_data.m_v6.sin6_addr = t_address.to_v6().to_native();
            m_data.m_v6.sin6_scope_id = t_address.to_v6().scope_id();
        }
    }

    basic_endpoint(const address_v4& t_address, std::uint16_t t_port) noexcept {
        m_data.m_v4.sin_family = AF_INET;
        m_data.m_v4.sin_port = htons(t_port);
        m_data.m_v4.sin_addr = t_address.to_native();
    }

    basic_endpoint(const address_v6& t_address, std::uint16_t t_port) noexcept {
        m_data.m_v6.sin6_family = AF_INET6;
        m_data.m_v6.sin6_port = htons(t_port);
        m_data.m_v6.sin6_flowinfo = 0;
        m_data.m_v6.sin6_addr = t_address.to_native();
        m_data.m_v6.sin6_scope_id = t_address.scope_id();
    }

    basic_endpoint(const sockaddr_in& t_address) noexcept {
        m_data.m_v4 = t_address;
    }

    basic_endpoint(const sockaddr_in6& t_address) noexcept {
        m_data.m_v6 = t_address;
    }

    [[nodiscard]] protocol_type protocol() const noexcept {
        if (m_data.m_v4.sin_family == AF_INET6) {
            return protocol_type::v6();
        }
        return protocol_type::v4();
    }

    [[nodiscard]] ip::address get_address() const noexcept {
        if (m_data.m_v4.sin_family == AF_INET6) {
            return ip::address{
                address_v6{m_data.m_v6.sin6_addr, m_data.m_v6.sin6_scope_id}
            };
        }
        return ip::address{address_v4{m_data.m_v4.sin_addr}};
    }

    [[nodiscard]] std::uint16_t get_port() const noexcept {
        if (m_data.m_v4.sin_family == AF_INET6) {
            return ntohs(m_data.m_v6.sin6_port);
        }
        return ntohs(m_data.m_v4.sin_port);
    }

    void set_port(std::uint16_t t_port) noexcept {
        if (m_data.m_v4.sin_family == AF_INET6) {
            m_data.m_v6.sin6_port = htons(t_port);
        } else {
            m_data.m_v4.sin_port = htons(t_port);
        }
    }

    [[nodiscard]] sockaddr* data() noexcept {
        return reinterpret_cast<sockaddr*>(&m_data);
    }

    [[nodiscard]] const sockaddr* data() const noexcept {
        return reinterpret_cast<const sockaddr*>(&m_data);
    }

    [[nodiscard]] std::size_t size() const noexcept {
        if (m_data.m_v4.sin_family == AF_INET6) {
            return sizeof(sockaddr_in6);
        }
        return sizeof(sockaddr_in);
    }

    void resize(std::size_t t_size) {
        if (t_size > capacity()) {
            throw std::length_error("endpoint size too large");
        }
        // Size is determined by address family, nothing to do
    }

    [[nodiscard]] std::size_t capacity() const noexcept {
        return sizeof(m_data);
    }

    bool operator==(const basic_endpoint& t_other) const noexcept {
        if (m_data.m_v4.sin_family != t_other.m_data.m_v4.sin_family) {
            return false;
        }
        if (m_data.m_v4.sin_family == AF_INET6) {
            return m_data.m_v6.sin6_port == t_other.m_data.m_v6.sin6_port &&
                   m_data.m_v6.sin6_flowinfo == t_other.m_data.m_v6.sin6_flowinfo &&
                   std::memcmp(&m_data.m_v6.sin6_addr, &t_other.m_data.m_v6.sin6_addr, sizeof(in6_addr)) == 0 &&
                   m_data.m_v6.sin6_scope_id == t_other.m_data.m_v6.sin6_scope_id;
        }
        return m_data.m_v4.sin_port == t_other.m_data.m_v4.sin_port &&
               m_data.m_v4.sin_addr.s_addr == t_other.m_data.m_v4.sin_addr.s_addr;
    }

    auto operator<=>(const basic_endpoint& t_other) const noexcept {
        // Compare address first, then port
        auto addr_cmp = get_address() <=> t_other.get_address();
        if (addr_cmp != std::strong_ordering::equal) {
            return addr_cmp;
        }
        return get_port() <=> t_other.get_port();
    }

private:
    union {
        sockaddr_in m_v4;
        sockaddr_in6 m_v6;
    } m_data{};
};
}  // namespace svarog::network::ip
