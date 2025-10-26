#pragma once

#include "svarog/ip/ip_address.hpp"
#include <iosfwd>
#include <format>

namespace svarog::ip
{
class ip_address_v4 : public ip_address
{
public:
    ip_address_v4() = default;
    ip_address_v4(const ip_address_v4&) = default;
    ip_address_v4(ip_address_v4&&) = default;

    ip_address_v4(const in_addr& t_address) : m_address(t_address) {}

    ip_address_v4& operator=(const ip_address_v4&) = default;
    ip_address_v4& operator=(ip_address_v4&&) = default;

    bool is_loopback() const override;
    bool is_multicast() const override;
    bool is_unspecified() const override;

    bool is_v4() const override;
    bool is_v6() const override;

    std::string to_string() const override;

private:
    in_addr m_address{ INADDR_NONE };
};

ip_address_v4 make_ip_address_v4(const char* t_address);
ip_address_v4 make_ip_address_v4(const std::string& t_address);
ip_address_v4 make_ip_address_v4(std::string_view t_address);

std::ostream& operator<<(std::ostream& os, const ip_address_v4& addr);
}

template <>
struct std::formatter<svarog::ip::ip_address_v4> : std::formatter<std::string>
{
    auto format(const svarog::ip::ip_address_v4& addr, std::format_context& ctx) const
    {
        return std::formatter<std::string>::format(addr.to_string(), ctx);
    }
};
