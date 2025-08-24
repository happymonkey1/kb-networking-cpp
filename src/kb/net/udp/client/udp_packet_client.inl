//
// Created by happy on 8/24/2025.
//

#ifndef KB_NETWORKING_UDP_PACKET_CLIENT_INL
#define KB_NETWORKING_UDP_PACKET_CLIENT_INL

#include "kb/kb_core.h"

namespace kb::net {

template <serialization_type_t SerdeT>
auto UdpPacketClient<SerdeT>::bind_packet_handler(
  packet_type_t p_packet_type,
  packet_handler_func_t&& p_handler
  ) noexcept -> bool {
  std::scoped_lock lock{ m_handler_mutex };
  if (m_handlers.contains(p_packet_type)) {
    return false;
  }

  m_handlers.emplace(p_packet_type, std::move(p_handler));
  return true;
}

template <serialization_type_t SerdeT>
auto UdpPacketClient<SerdeT>::unbind_packet_handler(packet_type_t p_packet_type) noexcept -> void {
  std::scoped_lock lock{ m_handler_mutex };
  m_handlers.erase(p_packet_type);
}

template <serialization_type_t SerdeT>
auto UdpPacketClient<SerdeT>::send_packet_internal(
  const details::internal_packet_t & p_packet,
  const bool p_reliable /* = true */
) const noexcept -> bool {
  return m_client.send_raw(p_packet.m_buffer.data(), p_packet.m_buffer.size(), p_reliable);
}

template <serialization_type_t SerdeT>
template <typename T>
auto UdpPacketClient<SerdeT>::send_packet(
  const packet_type_t   p_packet_type,
  const T             & p_object,
  const bool            p_reliable
  ) noexcept -> bool {
  auto packet = m_serializer.serialize_packet(k_invalid_connection, p_packet_type, p_object);
  return send_packet_internal(packet, p_reliable);
}

template <serialization_type_t SerdeT>
auto UdpPacketClient<SerdeT>::handle_message(incoming_view_message_t p_message) noexcept -> void {
  auto maybe_internal_packet_header = details::unpack_internal_packet_header(
    p_message.m_buffer.data(),
    p_message.m_buffer.size()
  );

  if (!maybe_internal_packet_header) {
    KB_LOG_WARN("UdpClient received malformed packet from conn: {}", p_message.m_conn);
    return;
  }

  auto header = std::move(*maybe_internal_packet_header);
  const auto packet_type = header.m_packet_type;

  typename std::unordered_map<packet_type_t , packet_handler_func_t>::iterator handler_it;
  {
    std::scoped_lock { m_handler_mutex };
    handler_it = m_handlers.find(packet_type);
    if (handler_it == m_handlers.end()) {
      KB_LOG_WARN(
        "[UdpPacketClient] Found valid packet header but could not find handler for packet type: {}",
        static_cast<u32>(packet_type)
      );
      return;
    }
  }

  // Use template magic to deserialize into an option< PayloadT >
  const auto * payload_ptr = static_cast<const char *>(p_message.m_buffer.data()) + sizeof(details::internal_packet_header_t);
  const auto payload_size = p_message.m_buffer.size() - sizeof(details::internal_packet_header_t);
  const auto maybe_payload = m_serializer.deserialize_packet(
    payload_ptr,
    payload_size
  );
  if (!maybe_payload) {
    return;
  }

  // Invoke the packet handler we located earlier with the deserialized maybe_payload
  auto & payload = *maybe_payload;
  handler_it->second(payload);
}

} // end namespace kb::net

#endif  //KB_NETWORKING_UDP_PACKET_CLIENT_H
