//
// Created by happy on 8/20/2025.
//

#ifndef KB_NETWORKING_ASYNC_UDP_PACKET_CLIENT_H
#define KB_NETWORKING_ASYNC_UDP_PACKET_CLIENT_H

#include "kb/types.h"
#include "kb/kb_networking_cpp.hpp"
#include "kb/net/network_types.h"
#include "kb/net/udp/packet.hpp"

#include <msgpack.hpp>
#include <coro/coro.hpp>

#include <atomic>
#include <functional>
#include <future>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

namespace kb::net {

template <serialization_type_t SerdeT>
class AsyncUdpPacketClient {
public:
  using IncomingMessage = ::kb::net::incoming_message_t<::kb::core::OwningBuffer>;
public:

  [[nodiscard]] static auto create() -> std::shared_ptr<AsyncUdpPacketClient> {
    return std::make_shared<AsyncUdpPacketClient>();
  }

  // Asynchronously initiate a connection.
  // Waits for a timeout period to ensure that connection is successful.
  [[nodiscard]] auto connect(const std::string & p_address, u16 p_port) noexcept -> coro::task<bool>;

  auto stop() noexcept -> void {
    return m_client.stop();
  }

  auto send_packet(kb_packet_type_t p_packet_type, auto&& p_object, bool p_reliable = true) noexcept -> bool {
    return m_client.send_packet(p_packet_type, p_object, p_reliable);
  }

  template <typename T>
  auto async_wait_for_packet(packet_type_t p_packet_type) noexcept -> coro::task<std::optional<T>>;

  auto get_connection_status() const noexcept -> UdpClient::connection_status_t { return m_client.get_connection_status(); }

  auto is_running() const noexcept -> bool { return m_client.is_running(); }

private:

  struct awaiting_packet_t {
    coro::event                    * m_event;
    std::optional<msgpack::object>   m_data;
  };

  auto register_packet_awaiter(
    packet_type_t     p_packet_type,
    awaiting_packet_t p_awaiting
  ) noexcept -> void;
  auto get_packet_awaiter(packet_type_t p_packet_type) noexcept -> std::optional<awaiting_packet_t>;
private:
  UdpPacketClient<SerdeT> m_client;

  // TODO: expose configuration through constructor
  // Reference: https://github.com/jbaldwin/libcoro?tab=readme-ov-file#thread_pool
  std::shared_ptr<coro::thread_pool> m_scheduler = coro::thread_pool::make_shared({
    .thread_count = 2,
    .on_thread_start_functor = nullptr,
    .on_thread_stop_functor = nullptr,
  });

  std::mutex m_message_queue_mutex;
  std::vector<IncomingMessage> m_message_queue;

  // Async awaiters
  std::mutex m_awaiters_mutex;
  std::unordered_map<packet_type_t, std::queue<awaiting_packet_t>> m_awaiters;
};

template <serialization_type_t SerdeT>
template <typename T>
auto AsyncUdpPacketClient<SerdeT>::async_wait_for_packet(packet_type_t p_packet_type) noexcept
  -> coro::task<std::optional<T>> {
  // Immediately schedule on the executor
  co_await m_scheduler->schedule();

  // Create and register and event to notify when we receive the packet
  coro::event event;
  register_packet_awaiter(p_packet_type,
    {
      .m_event = &event,
      .m_data = std::nullopt,
    }
  );
  co_await event;

  // Unpack the awaiting state and try to return the packet data
  auto awaiter = get_packet_awaiter(p_packet_type);
  if (!awaiter.has_value() || !awaiter->m_data) {
    co_return std::nullopt;
  }

  auto object = std::move(*awaiter->m_data);

  T packet_data{};
  try {
    object.convert(packet_data);
  } catch (const std::exception& err) {
    KB_LOG_ERROR("Failed to convert packet data to concrete serializer_t: {}", err.what());
    co_return std::nullopt;
  }

  co_return std::make_optional(packet_data);
}

} // end namespace kb::net

#include "kb/net/udp/client/async_udp_packet_client.inl"

#endif  //KB_NETWORKING_ASYNC_UDP_PACKET_CLIENT_H
