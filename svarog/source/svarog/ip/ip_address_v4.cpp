#include "svarog/ip/ip_address_v4.hpp"

#include <array>
#include <ostream>

#include <arpa/inet.h>

namespace {
in_addr parse_ipv4_address(const char* t_address) {
    in_addr result;
    if (inet_pton(AF_INET, t_address, &result) != 1) {
        return in_addr{INADDR_NONE};
    }

    return result;
}
}  // namespace

namespace svarog::ip {
bool ip_address_v4::is_loopback() const {
    return m_address.s_addr == htonl(INADDR_LOOPBACK);
}

bool ip_address_v4::is_multicast() const {
    return IN_MULTICAST(htonl(m_address.s_addr));
}

bool ip_address_v4::is_unspecified() const {
    return m_address.s_addr == INADDR_NONE;
}

bool ip_address_v4::is_v4() const {
    return true;
}

bool ip_address_v4::is_v6() const {
    return false;
}

std::string ip_address_v4::to_string() const {
    std::array<char, INET_ADDRSTRLEN> buffer{};
    if (inet_ntop(AF_INET, &m_address, buffer.data(), buffer.size()) == nullptr) {
        return std::string{};
    }
    return std::string{buffer.data()};
}

ip_address_v4 make_ip_address_v4(const char* t_address) {
    return ip_address_v4(parse_ipv4_address(t_address));
}

ip_address_v4 make_ip_address_v4(const std::string& t_address) {
    return ip_address_v4(parse_ipv4_address(t_address.c_str()));
}

ip_address_v4 make_ip_address_v4(std::string_view t_address) {
    return ip_address_v4(parse_ipv4_address(t_address.data()));
}

std::ostream& operator<<(std::ostream& os, const ip_address_v4& addr) {
    return os << addr.to_string();
}
}  // namespace svarog::ip
