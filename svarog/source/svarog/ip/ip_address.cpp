#include "svarog/ip/ip_address.hpp"

namespace
{
in_addr_t parse_ipv4_address(const char* address)
{
    return INADDR_ANY;
}
}

namespace svarog::ip
{
// constexpr ip_address make_ip_address(const char* address)
// {
//     return ip_address();
// }
} // namespace svarog::ip
