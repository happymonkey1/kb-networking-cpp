#include "kb/net/network_utils.h"

#include "kb/kb_networking_cpp.hpp"

namespace kb::net::utils {

auto is_valid_ip_address(std::string_view p_ip_address) noexcept -> bool {
  if (p_ip_address.empty()) {
    return false;
  }

  SteamNetworkingIPAddr address;
  return address.ParseString(p_ip_address.data());
}


} // end namespace kb::net::utils