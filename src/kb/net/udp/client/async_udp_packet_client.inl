//
// Created by happy on 8/24/2025.
//

#ifndef KB_NETWORKING_ASYNC_UDP_PACKET_CLIENT_INL
#define KB_NETWORKING_ASYNC_UDP_PACKET_CLIENT_INL

namespace kb::net {

template <serialization_type_t SerdeT>
auto AsyncUdpPacketClient<SerdeT>::connect(const std::string& p_address, u16 p_port) noexcept -> coro::task<bool> {
  co_await m_scheduler->schedule();

  if (m_client->is_running()) {
    KB_LOG_WARN("[AsyncUdpPacketClient] client is already running");
    co_return false;
  }

  const auto res = m_client->connect(p_address, p_port);
  if (!res) {
    KB_LOG_ERROR("[AsyncUdpPacketClient] Failed to connect to server, returning early");
    co_return false;
  }

  constexpr u32 k_max_wait_ms = 1000;
  constexpr u32 k_sleep_duration = 50;
  constexpr u32 max_iters = k_max_wait_ms / k_sleep_duration;
  u32 iter = 0;
  UdpClient::connection_status_t connection_status;
  do {
    connection_status = m_client->get_connection_status();

    if (iter++ >= max_iters) {
      KB_LOG_WARN("[AsyncUdpPacketClient] Failed to connect to server after {} ms", k_sleep_duration);
      break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(k_sleep_duration));
  } while (connection_status == UdpClient::connection_status_t::connecting);

  KB_LOG_INFO("[AsyncUdpPacketClient] Successfully connected to server: {}:{}", p_address, p_port);
  co_return connection_status == UdpClient::connection_status_t::connected;
}

template <serialization_type_t SerdeT>
template <typename T>
auto AsyncUdpPacketClient<SerdeT>::async_wait_for_packet(packet_type_t p_packet_type) noexcept
  -> coro::task<std::optional<T>> {
  // Immediately schedule on the executor
  co_await m_scheduler->schedule();

  // Create and register and event to notify when we receive the packet
  coro::event event;
  register_packet_awaiter(
    p_packet_type,
    std::move(awaiting_packet_t {
      .m_event = &event,
      .m_data = std::nullopt,
    })
  );
  co_await event;

  // Unpack the awaiting state and try to return the packet data
  auto awaiter = get_packet_awaiter(p_packet_type);
  if (!awaiter.has_value() || !awaiter->m_data) {
    KB_LOG_TRACE("[AsyncUdpPacketClient] Returning failed awaiter response");
    co_return std::nullopt;
  }

  co_return m_client->m_serializer.template payload_as<T>(std::move(*awaiter->m_data));
}

template <serialization_type_t SerdeT>
auto AsyncUdpPacketClient<SerdeT>::on_data_received_pre_handler(kb::net::incoming_view_message_t p_message) noexcept {
  KB_LOG_DEBUG("[AsyncUdpPacketClient] Entered on_data_received_pre_handler");

  auto maybe_header = details::unpack_internal_packet_header(
    p_message.m_buffer.data(), p_message.m_buffer.size());
  if (!maybe_header) {
    KB_LOG_TRACE("[AsyncUdpPacketClient] Invalid packet header");
    return false;
  }

  auto header = *maybe_header;
  if (header.m_serde_type != SerdeT) {
    KB_LOG_TRACE("[AsyncUdpPacketClient] Invalid serialization type: {}", static_cast<u32>(header.m_serde_type));
    return false;
  }

  const auto packet_type = header.m_packet_type;
  KB_LOG_TRACE("[AsyncUdpPacketClient] Found valid internal header with packet type: {}", static_cast<u32>(packet_type));

  awaiting_packet_t * awaiter = nullptr;
  {
    std::scoped_lock lock{ m_awaiters_mutex };
    auto it = m_awaiters.find(packet_type);
    if (it == m_awaiters.end() || it->second.empty()) {
      KB_LOG_TRACE("[AsyncUdpPacketClient] Could not find a registered awaiter");
      return false;
    }
    awaiter = &it->second.front();
  }

  const auto* payload_ptr =
    static_cast<const char*>(p_message.m_buffer.data()) + sizeof(details::internal_packet_header_t);
  const auto  payload_size =
    p_message.m_buffer.size() - sizeof(details::internal_packet_header_t);

  auto maybe_payload = m_client->m_serializer.deserialize_packet(payload_ptr, payload_size);
  if (!maybe_payload) {
    KB_LOG_WARN("[AsyncUdpPacketClient] Found await and valid packet header but failed to deserialize payload");
    if (awaiter->m_event) {
      awaiter->m_event->set();
    }
    return true;
  }

  awaiter->m_data = std::move(*maybe_payload);
  KB_LOG_TRACE("[AsyncUdpPacketClient] Sending event notification for successfully deserialized payload");

  if (awaiter->m_event) {
    awaiter->m_event->set();
  }

  return true;
}

template <serialization_type_t SerdeT>
auto AsyncUdpPacketClient<SerdeT>::register_packet_awaiter(packet_type_t p_packet_type, awaiting_packet_t p_awaiting) noexcept -> void {
  std::scoped_lock lock{ m_awaiters_mutex };
  if (m_awaiters.contains(p_packet_type)) {
    auto& queue = m_awaiters[p_packet_type];
    queue.push(std::move(p_awaiting));
  } else {
    std::queue<awaiting_packet_t> queue;
    queue.push(std::move(p_awaiting));
    m_awaiters.emplace(p_packet_type, std::move(queue));
  }

  KB_LOG_TRACE("[AsyncUdpPacketClient] Registered awaiter for packet type: {}", static_cast<u32>(p_packet_type));
}

template <serialization_type_t SerdeT>
auto AsyncUdpPacketClient<SerdeT>::get_packet_awaiter(packet_type_t p_packet_type) noexcept -> std::optional<awaiting_packet_t> {
  std::scoped_lock lock{ m_awaiters_mutex };
  auto& queue = m_awaiters[p_packet_type];
  if (queue.empty()) {
    KB_LOG_TRACE(
      "[AsyncUdpPacketClient] No packet awaiters found for packet type: {}",
      static_cast<u32>(p_packet_type)
    );
    return std::nullopt;
  }

  auto awaiting = std::make_optional(std::move(queue.front()));
  queue.pop();
  KB_LOG_TRACE("[AsyncUdpPacketClient] Popping registered packet awaiter for packet type: {}", static_cast<u32>(p_packet_type));
  return std::move(awaiting);
}

} // end namespace kb::net

#endif  //KB_NETWORKING_ASYNC_UDP_PACKET_CLIENT_INL
