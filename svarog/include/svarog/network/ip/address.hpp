#pragma once
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>

#include "svarog/network/ip/address_v4.hpp"
#include "svarog/network/ip/address_v6.hpp"

#include <format>

namespace svarog::network::ip {

class bad_address_cast : public std::bad_cast {
public:
    const char* what() const noexcept override {
        return "bad address cast";
    }
};

class address {
public:
    constexpr address() noexcept : m_addr(address_v4{}) {
    }

    constexpr address(const address_v4& t_v4) noexcept : m_addr(t_v4) {
    }
    constexpr address(const address_v6& t_v6) noexcept : m_addr(t_v6) {
    }

    [[nodiscard]] constexpr bool is_v4() const noexcept {
        return std::holds_alternative<address_v4>(m_addr);
    }

    [[nodiscard]] constexpr bool is_v6() const noexcept {
        return std::holds_alternative<address_v6>(m_addr);
    }

    [[nodiscard]] constexpr const address_v4& to_v4() const {
        if (const auto* p = std::get_if<address_v4>(&m_addr))
            return *p;
        throw bad_address_cast{};
    }

    [[nodiscard]] constexpr const address_v6& to_v6() const {
        if (const auto* p = std::get_if<address_v6>(&m_addr))
            return *p;
        throw bad_address_cast{};
    }

    [[nodiscard]] std::string to_string() const {
        return std::visit([](const auto& t_a) { return t_a.to_string(); }, m_addr);
    }

    [[nodiscard]] constexpr bool is_unspecified() const noexcept {
        return std::visit([](const auto& t_a) { return t_a.is_unspecified(); }, m_addr);
    }

    [[nodiscard]] constexpr bool is_loopback() const noexcept {
        return std::visit([](const auto& t_a) { return t_a.is_loopback(); }, m_addr);
    }

    [[nodiscard]] constexpr bool is_multicast() const noexcept {
        return std::visit([](const auto& t_a) { return t_a.is_multicast(); }, m_addr);
    }

    static std::optional<address> from_string(std::string_view t_string) noexcept;

    constexpr bool operator==(const address&) const noexcept = default;

    constexpr auto operator<=>(const address& t_other) const noexcept {
        // v4 < v6
        if (is_v4() && t_other.is_v6())
            return std::strong_ordering::less;
        if (is_v6() && t_other.is_v4())
            return std::strong_ordering::greater;

        if (is_v4())
            return to_v4() <=> t_other.to_v4();
        return to_v6() <=> t_other.to_v6();
    }

private:
    std::variant<address_v4, address_v6> m_addr;
};

std::ostream& operator<<(std::ostream& t_os, const address& t_address);

inline address make_address(std::string_view t_string) {
    auto result = address::from_string(t_string);
    if (!result)
        throw std::invalid_argument("Invalid IP address: " + std::string(t_string));
    return *result;
}

inline address make_address(std::string_view t_string, std::error_code& t_ec) noexcept {
    auto result = address::from_string(t_string);
    if (!result) {
        t_ec = std::make_error_code(std::errc::invalid_argument);
        return {};
    }
    t_ec.clear();
    return *result;
}

}  // namespace svarog::network::ip

template <>
struct std::formatter<svarog::network::ip::address> : std::formatter<std::string> {
    auto format(const svarog::network::ip::address& t_addr, std::format_context& t_ctx) const {
        return std::formatter<std::string>::format(t_addr.to_string(), t_ctx);
    }
};

template <>
struct std::hash<svarog::network::ip::address> {
    std::size_t operator()(const svarog::network::ip::address& t_addr) const noexcept {
        if (t_addr.is_v4())
            return std::hash<svarog::network::ip::address_v4>{}(t_addr.to_v4());
        return std::hash<svarog::network::ip::address_v6>{}(t_addr.to_v6());
    }
};
