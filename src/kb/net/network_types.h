//
// Created by happymonkey1 on 8/18/25.
//

#ifndef KB_NETWORKING_NETWORK_TYPES_H
#define KB_NETWORKING_NETWORK_TYPES_H

#include "kb/net/kb_network_types.h"
#include "kb/types.h"

#include <span>

// Forward declarations
class ISteamNetworkingSockets;
struct SteamNetConnectionStatusChangedCallback_t;
struct SteamNetworkingMessage_t;
using ISteamNetworkingMessage = SteamNetworkingMessage_t;
struct SteamNetworkingIPAddr;

namespace kb::net {

using packet_type_t = u32;

constexpr kb_connection_t k_invalid_connection = KB_CONNECTION_INVALID;

enum class internal_packet_id_t : packet_type_t {
  unknown = KB_PACKET_ID_UNKNOWN,
  reserved = KB_PACKET_ID_RESERVED,
};

enum class serialization_type_t : u8 {
  bin     = 0,
  msgpack = 1,
  custom = 7,
};

constexpr auto k_default_serde_type = serialization_type_t::msgpack;

// Alias for HSteamListenSocket
using listen_socket_t = u32;
constexpr auto k_invalid_listen_socket = 0;
// Alias for HSteamNetPollGroup
using poll_group_t    = u32;
constexpr auto k_invalid_poll_group = 0;

// Incoming message buffer
template <typename T>
struct incoming_message_t {
  kb_connection_t    m_conn;
  T                  m_buffer;
};

// Non-owning, view network message
using incoming_view_message_t = incoming_message_t<std::span<const char>>;

// From steamnetworkingtypes.h
// Internal memory representation of either an ipv4 or ipv6 address
struct ip_address_t {
  ip_address_t() noexcept = default;
  explicit ip_address_t(const SteamNetworkingIPAddr & p_addr) noexcept;
  auto operator=(const SteamNetworkingIPAddr & p_addr) noexcept -> ip_address_t &;

  /// RFC4038, section 4.2
  struct IPv4MappedAddress {
    u64 m_8zeros;
    u16 m_0000;
    u16 m_ffff;
    u8 m_ip[ 4 ]; // NOTE: As bytes, i.e. network byte order
  };

  union
  {
    u8 m_ipv6[ 16 ];
    IPv4MappedAddress m_ipv4;
  };
  u16 m_port; // Host byte order
};

} // end namespace kb::net

#endif  //KB_NETWORKING_NETWORK_TYPES_H
