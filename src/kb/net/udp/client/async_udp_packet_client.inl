//
// Created by happy on 8/24/2025.
//

#ifndef KB_NETWORKING_ASYNC_UDP_PACKET_CLIENT_INL
#define KB_NETWORKING_ASYNC_UDP_PACKET_CLIENT_INL

namespace kb::net {

template <serialization_type_t SerdeT>
auto AsyncUdpPacketClient<SerdeT>::connect(const std::string& p_address, u16 p_port) noexcept -> coro::task<bool> {
  co_await m_scheduler->schedule();

  if (m_client.is_running()) {
    KB_LOG_WARN("[AsyncUdpPacketClient] client is already running");
    co_return false;
  }

  const auto res = m_client.connect(p_address, p_port);
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
    connection_status = m_client.get_connection_status();

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
auto AsyncUdpPacketClient<SerdeT>::register_packet_awaiter(packet_type_t p_packet_type, awaiting_packet_t p_awaiting) noexcept -> void {
  std::scoped_lock lock{m_awaiters_mutex};
  if (m_awaiters.contains(p_packet_type)) {
    auto& queue = m_awaiters[p_packet_type];
    queue.push(p_awaiting);
  } else {
    std::queue<awaiting_packet_t> queue;
    queue.push(p_awaiting);
    m_awaiters.emplace(p_packet_type, std::move(queue));
  }
}

template <serialization_type_t SerdeT>
auto AsyncUdpPacketClient<SerdeT>::get_packet_awaiter(packet_type_t p_packet_type) noexcept -> std::optional<awaiting_packet_t> {
  std::scoped_lock lock{m_awaiters_mutex};
  auto& queue = m_awaiters[p_packet_type];
  if (queue.empty()) {
    return std::nullopt;
  }

  const auto awaiting = std::make_optional(std::move(queue.front()));
  queue.pop();
  return std::move(awaiting);
}

}

#endif  //KB_NETWORKING_ASYNC_UDP_PACKET_CLIENT_INL
