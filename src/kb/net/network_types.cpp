//
// Created by happy on 8/23/2025.
//

#include "kb/net/network_types.h"

#include <steam/steamnetworkingtypes.h>

#include <cstring>

namespace kb::net {

auto copy_steam_to_ip_address(ip_address_t & p_kb_addr, const SteamNetworkingIPAddr & p_steam_addr) noexcept -> void {
  p_kb_addr.m_port = p_steam_addr.m_port;
  if (p_steam_addr.IsIPv4()) {
    std::memcpy(&p_kb_addr.m_ipv4, &p_steam_addr.m_ipv4, sizeof(p_steam_addr.m_ipv4));
  } else {
    std::memcpy(&p_kb_addr.m_ipv6, &p_steam_addr.m_ipv6, sizeof(p_steam_addr.m_ipv6));
  }
}

ip_address_t::ip_address_t(const SteamNetworkingIPAddr & p_addr) noexcept
  : m_port{ p_addr.m_port }
{
  copy_steam_to_ip_address(*this, p_addr);
}

auto ip_address_t::operator=(const SteamNetworkingIPAddr & p_addr) noexcept -> ip_address_t& {
  copy_steam_to_ip_address(*this, p_addr);

  return *this;
}

} // end namespace kb::net