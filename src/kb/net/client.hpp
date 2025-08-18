//
// Created by happymonkey1 on 8/17/25.
//

#ifndef KB_NETWORKING_CLIENT_HPP
#define KB_NETWORKING_CLIENT_HPP

#include "kb/async/awaitable.hpp"
#include "kb/kb_networking_cpp.hpp"
#include "kb/net/packet.hpp"
#include "kb/types.h"

#include <steam/steamnetworkingtypes.h>

#include <coro/coro.hpp>
#include <msgpack.hpp>

#include <atomic>
#include <functional>
#include <future>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

namespace kb::net {

class Client {
public:
  using packet_handler_func_t = std::function<void(const msgpack::object& payload)>;

  static constexpr size_t k_default_poll_message_count = 16;
  static constexpr size_t k_default_poll_delay_ms = 10;

  enum class connection_status_t {
    disconnected = 0,
    connecting = 1,
    connected = 2,
    failed_to_connect = 3,
  };

public:
  ~Client() noexcept;

  auto stop() noexcept -> void;

  auto poll() noexcept -> void;

  auto is_running() const noexcept -> bool {
    return m_running.load();
  }

  auto connect(const std::string& p_address, u16 p_port) noexcept -> bool;
  auto async_connect(const std::string& p_address, u16 p_port) noexcept -> coro::task<bool>;

  auto try_receive_raw(msgpack::object_handle& p_out_object, HSteamNetConnection& p_out_conn) noexcept -> void;

  template <typename T>
  auto send_packet(packet_type_t p_packet_type, const T& p_object, bool p_reliable = true) noexcept -> bool;
  auto send_raw(const void * p_data, size_t p_len, bool p_reliable = true) const noexcept -> bool;

  auto bind_handler(packet_type_t p_packet_type, packet_handler_func_t&& p_handler) noexcept -> bool;
  auto unbind_handler(packet_type_t p_packet_type) noexcept -> void;

  auto async_wait_for_packet(packet_type_t p_packet_type) noexcept -> coro::task<std::optional<msgpack::object>>;

  auto get_connection_status() const noexcept -> connection_status_t {
    return m_connection_status;
  }
private:
  auto try_connect(const std::string& p_address, u16 p_port) noexcept -> bool;

  auto process_incoming_messages() noexcept -> void;
  auto poll_messages() noexcept -> void;
  auto push_message_to_queue(HSteamNetConnection p_conn, const void * p_data, size_t p_len) noexcept -> void;
  auto network_loop() noexcept -> void;

  struct awaiting_packet_t {
    coro::event                    * m_event;
    std::optional<msgpack::object>   m_data;
  };

  auto register_packet_awaiter(
    packet_type_t     p_packet_type,
    awaiting_packet_t p_awaiting
  ) noexcept -> void;
  auto get_packet_awaiter(packet_type_t p_packet_type) noexcept -> std::optional<awaiting_packet_t>;

  static auto connection_status_changed_callback(SteamNetConnectionStatusChangedCallback_t * p_info) noexcept -> void;
  auto on_connection_status_changed(SteamNetConnectionStatusChangedCallback_t * p_info) noexcept -> void;

private:
  friend struct awaitable_packet_t;

  // TODO: expose through constructor
  // Reference: https://github.com/jbaldwin/libcoro?tab=readme-ov-file#thread_pool
  std::shared_ptr<coro::thread_pool> m_scheduler = coro::thread_pool::make_shared({
    .thread_count = 2,
    .on_thread_start_functor = nullptr,
    .on_thread_stop_functor = nullptr,
  });

  std::atomic<bool> m_running{ false };
  connection_status_t m_connection_status = connection_status_t::disconnected;

  std::thread m_network_thread;

  // Incoming message buffer
  struct incoming_message_t {
    HSteamNetConnection m_conn;
    std::vector<char> m_data;
  };
  std::mutex m_message_queue_mutex;
  std::vector<incoming_message_t> m_message_queue;

  // Async awaiters
  std::mutex m_awaiters_mutex;
  std::unordered_map<packet_type_t, std::queue<awaiting_packet_t>> m_awaiters;

  // Registered packet handlers
  std::mutex m_handler_mutex;
  std::unordered_map<packet_type_t, packet_handler_func_t> m_handlers;

  ISteamNetworkingSockets* m_interface = nullptr;
  HSteamNetConnection m_conn = k_HSteamNetConnection_Invalid;
};

template <typename T>
auto Client::send_packet(const packet_type_t p_packet_type, const T& p_object,
                         const bool p_reliable) noexcept -> bool {
  msgpack::sbuffer buffer;
  msgpack::packer packer{ &buffer };
  packer.pack_array(2);
  packer.pack(p_packet_type);
  packer.pack(p_object);

  return send_raw(buffer.data(), buffer.size(), p_reliable);
}

}  // end namespace kb::net

#endif  //KB_NETWORKING_CLIENT_HPP
