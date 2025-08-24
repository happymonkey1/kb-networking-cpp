//
// Created by happymonkey1 on 8/17/25.
//

#ifndef KB_NETWORKING_UDP_CLIENT_HPP
#define KB_NETWORKING_UDP_CLIENT_HPP

#include "kb/net/network_types.h"
#include "kb/net/udp/packet.hpp"
#include "kb/types.h"

#include <atomic>
#include <functional>
#include <future>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

namespace kb::net {

class UdpClient {
public:
  using on_data_received_callback_func_t = std::function<void(incoming_view_message_t p_messages)>;

  static constexpr size_t k_default_poll_message_count = 16;
  static constexpr size_t k_default_poll_delay_ms = 10;

  enum class connection_status_t {
    disconnected = 0,
    connecting = 1,
    connected = 2,
    failed_to_connect = 3,
  };

public:
  ~UdpClient() noexcept;

  auto stop() noexcept -> void;

  auto poll() noexcept -> void;

  [[nodiscard]] auto is_running() const noexcept -> bool {
    return m_running.load();
  }

  // Try to connect to the given address and port.
  // Manually check connection status with `get_connection_status`
  // to ensure connection is successful.
  auto connect(const std::string & p_address, u16 p_port) noexcept -> bool;

  auto send_raw(const void * p_data, size_t p_len, bool p_reliable = true) const noexcept -> bool;

  auto set_on_data_callback(on_data_received_callback_func_t && p_on_data_received) noexcept -> void {
    m_callbacks.m_on_data_received = p_on_data_received;
  }

  [[nodiscard]] auto get_connection_status() const noexcept -> connection_status_t { return m_connection_status; }
private:
  auto try_connect(const std::string & p_address, u16 p_port) noexcept -> bool;

  auto poll_messages() noexcept -> void;

  // Internal network loop running on a dedicated thread
  auto network_loop() noexcept -> void;

  static auto connection_status_changed_callback(SteamNetConnectionStatusChangedCallback_t * p_info) noexcept -> void;
  auto on_connection_status_changed(SteamNetConnectionStatusChangedCallback_t * p_info) noexcept -> void;

private:
  std::atomic<bool> m_running{ false };
  connection_status_t m_connection_status = connection_status_t::disconnected;

  std::thread m_network_thread;

  struct callbacks_t {
    on_data_received_callback_func_t m_on_data_received = nullptr;
  };

  callbacks_t m_callbacks{};

  ISteamNetworkingSockets * m_interface = nullptr;
  kb_connection_t m_conn = k_invalid_connection;
};

} // end namespace kb::net

#endif  //KB_NETWORKING_UDP_CLIENT_HPP
