//
// Created by happy on 8/23/2025.
//

#ifndef KB_NETWORKING_UDP_HPP
#define KB_NETWORKING_UDP_HPP

#include "kb/net/udp/server/udp_packet_server.hpp"
#include "kb/net/udp/client/udp_packet_client.hpp"
#include "kb/net/udp/client/async_udp_packet_client.h"

namespace kb::net::udp {

inline auto create_binary_server() noexcept -> std::shared_ptr<UdpPacketServer<serialization_type_t::bin>> {
  return UdpPacketServer<serialization_type_t::bin>::create();
}

inline auto create_msgpack_server() noexcept -> std::shared_ptr<UdpPacketServer<serialization_type_t::msgpack>> {
  return UdpPacketServer<serialization_type_t::msgpack>::create();
}

inline auto create_binary_client() noexcept -> std::shared_ptr<UdpPacketClient<serialization_type_t::bin>> {
  return UdpPacketClient<serialization_type_t::bin>::create();
}

inline auto create_msgpack_client() noexcept -> std::shared_ptr<UdpPacketClient<serialization_type_t::msgpack>> {
  return UdpPacketClient<serialization_type_t::msgpack>::create();
}

inline auto create_async_binary_client() noexcept -> std::shared_ptr<AsyncUdpPacketClient<serialization_type_t::bin>> {
  return AsyncUdpPacketClient<serialization_type_t::bin>::create();
}

inline auto create_async_msgpack_client() noexcept -> std::shared_ptr<AsyncUdpPacketClient<serialization_type_t::msgpack>> {
  return AsyncUdpPacketClient<serialization_type_t::msgpack>::create();
}

} // end namespace kb::net::udp

#endif  //KB_NETWORKING_UDP_HPP
