//
// Created by happymonkey1 on 8/17/25.
//

#ifndef KB_NETWORKING_CLIENT_HPP
#define KB_NETWORKING_CLIENT_HPP
#include <steam/steamnetworkingtypes.h>

#include <msgpack.hpp>

#include <atomic>
#include <thread>
#include <unordered_map>
#include <vector>
#include <functional>

#include "kb/async/awaitable.hpp"
#include "kb/net/packet.h"
#include "kb/types.h"

namespace kb::net {

class Client {
public:
  using packet_handler_func_t = std::function<void(const msgpack::object& payload)>;

public:

  ~Client() noexcept;

  auto start_async() noexcept -> bool;
  auto stop() noexcept -> void;

  auto start_manual() noexcept -> bool;
  auto poll() noexcept -> void;

  auto is_running() const noexcept -> bool {
    return m_running.load();
  }

  auto connect(const std::string& p_address, u16 p_port) noexcept -> bool;
  auto async_connect(const std::string& p_address, u16 p_port) noexcept -> awaitable<bool>;

  auto try_receive_raw(msgpack::object_handle& p_out_object, HSteamNetConnection& p_out_conn) noexcept -> void;

  template <typename T>
  auto send_packet(packet_type_t p_packet_type, const T& p_object, bool p_reliable = true) noexcept -> bool;
  auto send_raw(const void * p_data, size_t p_len, bool p_reliable = true) noexcept -> bool;

  auto bind_handler(packet_type_t p_packet_type, packet_handler_func_t&& p_handler);
  auto unbind_handler(packet_type_t p_packet_type) -> void;

  auto async_wait_for_packet(packet_type_t p_packet_type) noexcept -> awaitable<msgpack::object_handle>;
  auto async_send_packet(packet_type_t p_packet_type, msgpack::object& p_object, bool p_reliable = true) noexcept -> bool;
private:
  std::atomic<bool> m_running{ false };

  std::thread m_network_thread;

  struct incoming_message_t {
    HSteamNetConnection m_conn;
    std::vector<char> m_data;
  };
  std::mutex m_message_queue_mutex;
  std::vector<incoming_message_t> m_message_queue;

  std::mutex m_handler_mutex;
  std::unordered_map<packet_type_t, packet_handler_func_t> m_handlers;

};

}

#endif  //KB_NETWORKING_CLIENT_HPP
