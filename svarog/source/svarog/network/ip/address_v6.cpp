#include "svarog/network/ip/address_v6.hpp"

#include <algorithm>
#include <cstring>
#include <sstream>

#include "svarog/network/ip/address.hpp"
#include "svarog/network/ip/address_v4.hpp"

namespace svarog::network::ip {

address_v6::address_v6(const in6_addr& t_address, std::uint32_t t_scope) noexcept : m_scope_id(t_scope) {
    std::memcpy(m_bytes.data(), &t_address.s6_addr, 16);
}

in6_addr address_v6::to_native() const noexcept {
    in6_addr result{};
    std::memcpy(&result.s6_addr, m_bytes.data(), 16);
    return result;
}

std::string address_v6::to_string() const {
    std::array<char, INET6_ADDRSTRLEN + 16> buf{};  // +16 for %scope
    in6_addr native = to_native();

    if (inet_ntop(AF_INET6, &native, buf.data(), INET6_ADDRSTRLEN) == nullptr) {
        return "::";  // Fallback
    }

    std::string result{buf.data()};
    if (m_scope_id != 0) {
        result += '%';
        result += std::to_string(m_scope_id);
    }
    return result;
}

address_v4 address_v6::to_v4() const {
    if (!is_v4_mapped() && !is_v4_compatible()) {
        throw bad_address_cast{};
    }
    return address_v4{
        address_v4::bytes_type{m_bytes[12], m_bytes[13], m_bytes[14], m_bytes[15]}
    };
}

address_v6 address_v6::v4_mapped(const address_v4& t_v4) noexcept {
    auto v4_bytes = t_v4.to_bytes();
    bytes_type bytes{};
    bytes[10] = 0xFF;
    bytes[11] = 0xFF;
    bytes[12] = v4_bytes[0];
    bytes[13] = v4_bytes[1];
    bytes[14] = v4_bytes[2];
    bytes[15] = v4_bytes[3];
    return address_v6{bytes};
}

std::optional<address_v6> address_v6::from_string(std::string_view t_string) noexcept {
    // Handle scope ID
    std::uint32_t scope_id = 0;
    auto scope_pos = t_string.find('%');
    std::string addr_part;

    if (scope_pos != std::string_view::npos) {
        addr_part = std::string(t_string.substr(0, scope_pos));
        auto scope_str = t_string.substr(scope_pos + 1);
        // Try to parse scope as number
        try {
            scope_id = static_cast<std::uint32_t>(std::stoul(std::string(scope_str)));
        } catch (...) {
            // Could be interface name - for now just ignore
        }
    } else {
        addr_part = std::string(t_string);
    }

    in6_addr t_address{};
    if (inet_pton(AF_INET6, addr_part.c_str(), &t_address) != 1) {
        return std::nullopt;
    }
    return address_v6{t_address, scope_id};
}

std::ostream& operator<<(std::ostream& t_os, const address_v6& t_address) {
    return t_os << t_address.to_string();
}

}  // namespace svarog::network::ip

std::size_t std::hash<svarog::network::ip::address_v6>::operator()(
    const svarog::network::ip::address_v6& t_address) const noexcept {
    auto bytes = t_address.to_bytes();
    std::size_t h = 0;
    for (auto b : bytes) {
        h ^= std::hash<std::uint8_t>{}(b) + 0x9e3779b9 + (h << 6) + (h >> 2);
    }
    h ^= std::hash<std::uint32_t>{}(t_address.scope_id());
    return h;
}
