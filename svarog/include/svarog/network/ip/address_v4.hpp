#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "svarog/network/detail/socket_types.hpp"

#include <format>

namespace svarog::network::ip {
class address_v4 {
public:
    using bytes_type = std::array<std::uint8_t, 4>;
    using uint_type = std::uint32_t;

    constexpr address_v4() noexcept = default;

    constexpr explicit address_v4(uint_type t_address) noexcept : m_address(t_address) {
    }

    constexpr explicit address_v4(const bytes_type& t_bytes) noexcept
        : m_address(static_cast<uint_type>(t_bytes[0] << 24) | static_cast<uint_type>(t_bytes[1] << 16) |
                    static_cast<uint_type>(t_bytes[2] << 8) | static_cast<uint_type>(t_bytes[3])) {
    }

    explicit address_v4(const in_addr& t_address) noexcept;

    [[nodiscard]] constexpr uint_type to_uint() const noexcept {
        return m_address;
    }

    [[nodiscard]] constexpr bytes_type to_bytes() const noexcept {
        return {
            static_cast<std::uint8_t>((m_address >> 24) & 0xFF),
            static_cast<std::uint8_t>((m_address >> 16) & 0xFF),
            static_cast<std::uint8_t>((m_address >> 8) & 0xFF),
            static_cast<std::uint8_t>((m_address) & 0xFF),
        };
    }

    [[nodiscard]] in_addr to_native() const noexcept;

    [[nodiscard]] std::string to_string() const;

    [[nodiscard]] constexpr bool is_unspecified() const noexcept {
        return m_address == 0;
    }

    [[nodiscard]] constexpr bool is_loopback() const noexcept {
        return (m_address & 0xFF000000) == 0x7F000000;
    }

    [[nodiscard]] constexpr bool is_multicast() const noexcept {
        return (m_address & 0xF0000000) == 0xE0000000;
    }

    [[nodiscard]] constexpr bool is_private() const noexcept {
        if ((m_address & 0xFF000000) == 0x0A000000)
            return true;

        if ((m_address & 0xFFF00000) == 0xAC100000)
            return true;

        return (m_address & 0xFFFF0000) == 0xC0A80000;
    }

    [[nodiscard]] constexpr bool is_link_local() const noexcept {
        return (m_address & 0xFFFF0000) == 0xA9FE0000;
    }

    static constexpr address_v4 any() noexcept {
        return address_v4{uint_type{0}};
    }

    static constexpr address_v4 loopback() noexcept {
        return address_v4{uint_type{0x7F000001}};
    }

    static constexpr address_v4 broadcast() noexcept {
        return address_v4{uint_type{0xFFFFFFFF}};
    }

    static std::optional<address_v4> from_string(std::string_view t_string) noexcept;

    constexpr bool operator==(const address_v4&) const noexcept = default;
    constexpr auto operator<=>(const address_v4&) const noexcept = default;

private:
    uint_type m_address{};
};

std::ostream& operator<<(std::ostream& t_os, const address_v4& t_address);

}  // namespace svarog::network::ip

template <>
struct std::formatter<svarog::network::ip::address_v4> : std::formatter<std::string> {
    auto format(const svarog::network::ip::address_v4& t_address, std::format_context& t_context) const {
        return std::formatter<std::string>::format(t_address.to_string(), t_context);
    }
};

template <>
struct std::hash<svarog::network::ip::address_v4> {
    std::size_t operator()(const svarog::network::ip::address_v4& t_address) const noexcept {
        return std::hash<std::uint32_t>{}(t_address.to_uint());
    }
};
