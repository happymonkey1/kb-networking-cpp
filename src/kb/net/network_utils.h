#ifndef KB_NETWORKING_NETWORKING_UTILS_H
#define KB_NETWORKING_NETWORKING_UTILS_H

#include <string_view>

namespace kb::net::utils {

auto is_valid_ip_address(std::string_view p_ip_address) noexcept -> bool;

} // end namespace kb::net::utils

#endif // KB_NETWORKING_NETWORKING_UTILS_H