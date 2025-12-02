#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "svarog/network/detail/socket_types.hpp"

#include <compare>
#include <format>

namespace svarog::network::ip {

class address_v4;

class address_v6 {
public:
    using bytes_type = std::array<std::uint8_t, 16>;

    constexpr address_v6() noexcept = default;

    constexpr explicit address_v6(const bytes_type& t_bytes, std::uint32_t t_scope = 0) noexcept
        : m_bytes(t_bytes), m_scope_id(t_scope) {
    }

    explicit address_v6(const in6_addr& t_address, std::uint32_t t_scope = 0) noexcept;

    [[nodiscard]] constexpr bytes_type to_bytes() const noexcept {
        return m_bytes;
    }

    [[nodiscard]] in6_addr to_native() const noexcept;

    [[nodiscard]] std::string to_string() const;

    [[nodiscard]] constexpr std::uint32_t scope_id() const noexcept {
        return m_scope_id;
    }
    constexpr void scope_id(std::uint32_t t_id) noexcept {
        m_scope_id = t_id;
    }

    [[nodiscard]] constexpr bool is_unspecified() const noexcept {
        return std::ranges::all_of(m_bytes, [](std::uint8_t t_element) { return t_element == 0; });
    }

    [[nodiscard]] constexpr bool is_loopback() const noexcept {
        // ::1
        for (std::size_t i = 0; i < 15; ++i) {
            if (m_bytes[i] != 0) {
                return false;
            }
        }

        return m_bytes[15] == 1;
    }

    [[nodiscard]] constexpr bool is_multicast() const noexcept {
        return m_bytes[0] == 0xFF;
    }

    [[nodiscard]] constexpr bool is_link_local() const noexcept {
        return m_bytes[0] == 0xFE && (m_bytes[1] & 0xC0) == 0x80;
    }

    [[nodiscard]] constexpr bool is_site_local() const noexcept {
        return m_bytes[0] == 0xFE && (m_bytes[1] & 0xC0) == 0xC0;
    }

    [[nodiscard]] constexpr bool is_v4_mapped() const noexcept {
        return m_bytes[0] == 0 && m_bytes[1] == 0 && m_bytes[2] == 0 && m_bytes[3] == 0 && m_bytes[4] == 0 &&
               m_bytes[5] == 0 && m_bytes[6] == 0 && m_bytes[7] == 0 && m_bytes[8] == 0 && m_bytes[9] == 0 &&
               m_bytes[10] == 0xFF && m_bytes[11] == 0xFF;
    }

    [[nodiscard]] constexpr bool is_v4_compatible() const noexcept {
        for (std::size_t i = 0; i < 12; ++i) {
            if (m_bytes[i] != 0) {
                return false;
            }
        }
        return !is_unspecified() && !is_loopback();
    }

    [[nodiscard]] address_v4 to_v4() const;

    static address_v6 v4_mapped(const address_v4& t_v4) noexcept;

    static constexpr address_v6 any() noexcept {
        return address_v6{};
    }

    static constexpr address_v6 loopback() noexcept {
        bytes_type bytes{};
        bytes[15] = 1;
        return address_v6{bytes};
    }

    static std::optional<address_v6> from_string(std::string_view t_string) noexcept;

    constexpr bool operator==(const address_v6& t_other) const noexcept {
        return m_bytes == t_other.m_bytes && m_scope_id == t_other.m_scope_id;
    }

    constexpr auto operator<=>(const address_v6& t_other) const noexcept {
        if (auto cmp = m_bytes <=> t_other.m_bytes; cmp != 0)
            return cmp;

        return m_scope_id <=> t_other.m_scope_id;
    }

private:
    bytes_type m_bytes{};
    std::uint32_t m_scope_id{0};
};

std::ostream& operator<<(std::ostream& t_os, const address_v6& t_address);

}  // namespace svarog::network::ip

template <>
struct std::formatter<svarog::network::ip::address_v6> : std::formatter<std::string> {
    auto format(const svarog::network::ip::address_v6& t_address, std::format_context& t_context) const {
        return std::formatter<std::string>::format(t_address.to_string(), t_context);
    }
};

template <>
struct std::hash<svarog::network::ip::address_v6> {
    std::size_t operator()(const svarog::network::ip::address_v6& t_address) const noexcept;
};
