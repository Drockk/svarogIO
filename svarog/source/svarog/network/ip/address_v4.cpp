#include "svarog/network/ip/address_v4.hpp"

#include <cstring>
#include <sstream>

namespace svarog::network::ip {

address_v4::address_v4(const in_addr& t_address) noexcept : m_address(ntohl(t_address.s_addr)) {
}

in_addr address_v4::to_native() const noexcept {
    in_addr result{};
    result.s_addr = htonl(m_address);
    return result;
}

std::string address_v4::to_string() const {
    auto bytes = to_bytes();
    return std::format("{}.{}.{}.{}", bytes[0], bytes[1], bytes[2], bytes[3]);
}

std::optional<address_v4> address_v4::from_string(std::string_view t_string) noexcept {
    // Null-terminate for inet_pton
    std::array<char, INET_ADDRSTRLEN + 1> buf{};
    if (t_string.size() >= sizeof(buf))
        return std::nullopt;
    std::memcpy(buf.data(), t_string.data(), t_string.size());
    buf.at(t_string.size()) = '\0';

    in_addr addr{};
    if (inet_pton(AF_INET, buf.data(), &addr) != 1) {
        return std::nullopt;
    }
    return address_v4{addr};
}

std::ostream& operator<<(std::ostream& t_os, const address_v4& t_address) {
    return t_os << t_address.to_string();
}

}  // namespace svarog::network::ip
