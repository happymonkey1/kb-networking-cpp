//
// Created by happy on 8/23/2025.
//

#ifndef KB_NETWORKING_UDP_PACKET_SERVER_INL
#define KB_NETWORKING_UDP_PACKET_SERVER_INL

namespace kb::net {

template <serialization_type_t SerdeT>
auto UdpPacketServer<SerdeT>::create() noexcept -> std::shared_ptr<UdpPacketServer> {
  const auto server = std::make_shared<UdpPacketServer>();

  // TODO: consider moving weak-callback wrapping into UdpServer itself
  server->m_server.set_on_data_callback(
    [weak = server->weak_from_this()](
      const client_info_t& client,
      const void* data,
      size_t size) {
      if (auto self = weak.lock()) {
        self->handle_packet(client, data, size);
      }
    }
  );

  return server;
}

template <serialization_type_t SerdeT>
auto UdpPacketServer<SerdeT>::bind_packet_handler(packet_type_t p_packet_type, UdpPacketServer::packet_handler_func_t&& packet_handler_func) noexcept -> bool {
  if (m_packet_handlers.contains(p_packet_type)) {
    KB_LOG_ERROR("[UdpServer] Failed to bind packet handler for packet serializer_t: {}. It is already bound!", p_packet_type);
    return false;
  }

  m_packet_handlers[p_packet_type] = packet_handler_func;
  KB_LOG_DEBUG("[UdpServer] Successfully bound packet handler for packet: {}", p_packet_type);
  return true;
}

template <serialization_type_t SerdeT>
template <typename T>
auto UdpPacketServer<SerdeT>::send(const kb_connection_t p_conn, const packet_type_t p_packet_type, T&& p_obj, const bool p_reliable) const noexcept -> bool {
  const auto packet = m_serializer.serialize_packet(p_conn, p_packet_type, std::forward<T>(p_obj));
  return send_packet_internal(packet, p_reliable);
}

template <serialization_type_t SerdeT>
template <typename T>
auto UdpPacketServer<SerdeT>::broadcast(const packet_type_t p_packet_type, T&& p_obj, const bool p_reliable) noexcept -> bool {
  const auto packet = m_serializer.serialize_packet(k_invalid_connection, p_packet_type, std::forward<T>(p_obj));
  return broadcast_packet_internal(packet, p_reliable);
}

template <serialization_type_t SerdeT>
auto UdpPacketServer<SerdeT>::unbind_packet_handler(const packet_type_t p_packet_type) noexcept
  -> void {
  m_packet_handlers.erase(p_packet_type);
}

template <serialization_type_t SerdeT>
auto UdpPacketServer<SerdeT>::handle_packet(
  const client_info_t & p_client,
  const void * p_data,
  const size_t p_len
) noexcept -> bool {
  KB_LOG_DEBUG("[UdpPacketServer] Entered handle packet");
  const auto conn = p_client.m_conn;

  const auto maybe_header = details::unpack_internal_packet_header(p_data, p_len);
  if (!maybe_header) {
    KB_LOG_WARN("[UdpPacketServer] Received invalid internal packet header from conn: {}", conn);
    return false;
  }

  auto header = *maybe_header;
  const auto serde_type = header.m_serde_type;
  if (serde_type != SerdeT) {
    KB_LOG_WARN(
      "[UdpPacketServer]: Successfully deserialized packet header but serde type is not valid: {}",
      static_cast<u32>(serde_type)
    );
    return false;
  }

  const auto packet_type = header.m_packet_type;

  auto handler_it = m_packet_handlers.find(packet_type);
  if (handler_it == m_packet_handlers.end()) {
    KB_LOG_ERROR(
      "[UdpPacketServer]: Deserialized packet but could not find a valid handler for packet type: {}",
      static_cast<u32>(packet_type)
    );
    return false;
  }

  const auto * payload_ptr = static_cast<const char *>(p_data) + sizeof(details::internal_packet_header_t);
  const auto payload_size = p_len - sizeof(details::internal_packet_header_t);
  KB_ASSERT(payload_size < p_len, "Preventing overflow with value: %d", payload_size);
  KB_LOG_TRACE("[UdpPacketServer]: Packet payload size: {} bytes", payload_size);

  const auto maybe_payload = m_serializer.deserialize_packet(payload_ptr, payload_size);
  if (!maybe_payload) {
    KB_LOG_WARN(
      "[UdpPacketServer]: Found valid internal packet header but failed to deserialize maybe_payload for packet type: {}",
      static_cast<u32>(packet_type)
    );
    return false;
  }

  auto & payload = *maybe_payload;
  handler_it->second(p_client.m_conn, payload);

  return true;
}

} // end namespace kb::net

#endif  //KB_NETWORKING_UDP_PACKET_SERVER_H
