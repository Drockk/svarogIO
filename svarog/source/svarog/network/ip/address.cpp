#include "svarog/network/ip/address.hpp"

namespace svarog::network::ip {

std::optional<address> address::from_string(std::string_view t_string) noexcept {
    if (auto v4 = address_v4::from_string(t_string)) {
        return address{*v4};
    }

    // Try IPv6
    if (auto v6 = address_v6::from_string(t_string)) {
        return address{*v6};
    }

    return std::nullopt;
}

std::ostream& operator<<(std::ostream& t_os, const address& t_address) {
    return t_os << t_address.to_string();
}

}  // namespace svarog::network::ip
