#pragma once

#include <string>
#include <utility>

#include <netinet/in.h>

namespace svarog::ip {
class ip_address {
public:
    virtual ~ip_address() = default;

    ip_address() = default;
    ip_address(const ip_address&) = default;
    ip_address(ip_address&&) = default;
    ip_address& operator=(const ip_address&) = default;
    ip_address& operator=(ip_address&&) = default;

    virtual bool is_loopback() const = 0;
    virtual bool is_multicast() const = 0;
    virtual bool is_unspecified() const = 0;

    virtual bool is_v4() const = 0;
    virtual bool is_v6() const = 0;

    virtual std::string to_string() const = 0;
};

// constexpr ip_address make_ip_address(const char* address);
// constexpr ip_address make_ip_address(const std::string& address);
// constexpr ip_address make_ip_address(std::string_view address);

}  // namespace svarog::ip
