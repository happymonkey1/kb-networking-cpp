//
// Created by happy on 8/19/2025.
//

#ifndef KB_NETWORKING_UDP_PACKET_SERVER_HPP
#define KB_NETWORKING_UDP_PACKET_SERVER_HPP

#include "kb/net/network_types.h"
#include "udp_server.hpp"
#include <msgpack.hpp>

namespace kb::net {

template <serialization_type_t SerdeT>
class UdpPacketServer : public std::enable_shared_from_this<UdpPacketServer<SerdeT>> {
public:
  using serde_traits = serde::serializer_traits<SerdeT>;
  using packet_handler_func_t = std::function<void(kb_connection_t p_conn, const typename serde_traits::payload_t & p_payload)>;
  using serializer_t = typename serde_traits::serializer_t;
public:

  static auto create() noexcept -> std::shared_ptr<UdpPacketServer>;

  auto start(u16 p_port) noexcept -> bool { return m_server.start_async(p_port); }
  auto stop() noexcept -> void { return m_server.stop(); }

  template <typename T>
  auto send(kb_connection_t p_conn, packet_type_t p_packet_type, T&& p_obj, bool p_reliable) const noexcept -> bool;

  template <typename T>
  auto broadcast(packet_type_t p_packet_type, T&& p_obj, bool p_reliable) noexcept -> bool;

  [[nodiscard]] auto bind_packet_handler(
    packet_type_t            p_packet_type,
    packet_handler_func_t && packet_handler_func
  ) noexcept -> bool;

  auto unbind_packet_handler(packet_type_t p_packet_type) noexcept -> void;

private:
  // Orchestrator for handling packets based on received packet serialization serializer_t
  auto handle_packet(const client_info_t & p_conn, const void * p_data, std::size_t p_size) noexcept -> bool;

  [[nodiscard]] auto send_packet_internal(
    const details::internal_packet_t & p_packet,
    const bool p_reliable = true) const -> bool {
    return m_server.send(p_packet.m_conn, p_packet.m_buffer.data(), static_cast<u32>(p_packet.m_buffer.size()), p_reliable);
  }

  [[nodiscard]] auto broadcast_packet_internal(
    const details::internal_packet_t & p_packet,
    const bool p_reliable = true) noexcept -> bool {
    return m_server.broadcast(p_packet.m_buffer.data(), static_cast<u32>(p_packet.m_buffer.size()), p_reliable);
  }

public:
  // Underlying server
  UdpServer m_server;
  // Map of packet dispatch handers based on incoming packet serializer_t
  std::unordered_map<packet_type_t, packet_handler_func_t> m_packet_handlers;
  // Packet serialization handler
  serializer_t m_serializer;
};

}

#include "kb/net/udp/server/udp_packet_server.inl"

#endif  //KB_NETWORKING_UDP_PACKET_SERVER_HPP
