//
// Created by happymonkey1 on 8/16/25.
//

#ifndef KB_NETWORKING_CPP_SERVER_H
#define KB_NETWORKING_CPP_SERVER_H

#include "../types.h"
#include "packet.h"

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>

#include <functional>
#ifndef STEAMNETWORKINGSOCKETS_OPENSOURCE
#include <steam/steam_api.h>
#endif

#include <msgpack.hpp>

#include <thread>
#include <map>

namespace kb::net {

struct client_info_t {
  HSteamNetConnection m_conn;
  SteamNetworkingIPAddr m_addr;
};

class Server {
public:
  using data_received_callback_func_t = std::function<void(const client_info_t&, const void*, size_t)>;
  using client_connected_callback_func_t = std::function<void(const client_info_t&)>;
  using client_disconnected_callback_func_t = std::function<void(const client_info_t&)>;

  static constexpr u32 k_default_poll_timeout = 10;

  using packet_handler_func_t = std::function<void(HSteamNetConnection p_conn, const msgpack::object& p_object)>;

public:
  Server();
  ~Server() noexcept;

  auto start_async(u16 p_port) noexcept -> bool;
  auto start_manual(u16 p_port) noexcept -> bool;
  auto stop() noexcept -> void;

  auto poll() noexcept -> void;

  template <typename T>
  [[nodiscard]] auto send(HSteamNetConnection p_conn, packet_type_t p_packet_type, const T& p_obj, bool p_reliable) const noexcept -> bool;
  [[nodiscard]] auto send_raw(HSteamNetConnection p_conn, const void *p_data, size_t p_len, bool p_reliable) const noexcept -> bool;

  template <typename T>
  [[nodiscard]] auto broadcast(packet_type_t p_packet_type, const T& p_obj, bool p_reliable) noexcept -> bool;
  [[nodiscard]] auto broadcast_raw(const void *p_data, size_t p_len, bool p_reliable) noexcept -> bool;

  auto disconnect(HSteamNetConnection p_conn, i32 p_reason = 0) noexcept -> void;

  [[nodiscard]] auto bind_packet_handler(packet_type_t p_packet_type, packet_handler_func_t&& packet_handler_func) noexcept -> bool;

  [[nodiscard]] auto is_running() const noexcept -> bool { return m_is_running.load(); }
  [[nodiscard]] auto port() const noexcept -> u16 { return m_port; }

  auto set_on_data_callback(const data_received_callback_func_t& p_call) noexcept -> void {
    m_data_received_callback = p_call;
  }

  auto set_on_client_connected_callback(const client_connected_callback_func_t& p_call) noexcept -> void {
    m_client_connected_callback = p_call;
  }

  auto set_on_client_disconnected_callback(const client_disconnected_callback_func_t& p_call) noexcept -> void {
    m_client_disconnected_callback = p_call;
  }

  [[nodiscard]] auto get_client_info(HSteamNetConnection p_conn) noexcept -> const client_info_t * {
    std::lock_guard lock{ m_client_mutex };
    const auto client = m_clients.find(p_conn);
    return client != m_clients.end() ? &client->second : nullptr;
  }

private:
  auto network_loop() noexcept -> void;
  static auto connection_status_changed_callback(SteamNetConnectionStatusChangedCallback_t *p_info) noexcept -> void;
  auto on_connection_status_changed(const SteamNetConnectionStatusChangedCallback_t *p_info) noexcept -> void;

  auto handle_packet(HSteamNetConnection p_conn, const void * p_data, size_t p_len) noexcept -> void;

  auto on_fatal_message(const char *p_msg) noexcept -> void;

private:
  std::thread m_network_thread;
  std::atomic<bool> m_is_running{ false };
  u16 m_port = 0;

  data_received_callback_func_t m_data_received_callback;
  client_connected_callback_func_t m_client_connected_callback;
  client_disconnected_callback_func_t m_client_disconnected_callback;

  std::map<HSteamNetConnection, client_info_t> m_clients;
  std::mutex m_client_mutex;

  // Map of packet dispatch handers based on incoming packet type
  std::unordered_map<packet_type_t, packet_handler_func_t> m_packet_handlers;

  ISteamNetworkingSockets *m_interface = nullptr;
  HSteamListenSocket m_listen_socket = k_HSteamListenSocket_Invalid;
  HSteamNetPollGroup m_poll_group = k_HSteamNetPollGroup_Invalid;
};

template <typename T>
auto Server::send(const HSteamNetConnection p_conn, const packet_type_t p_packet_type,
                  const T& p_obj, const bool p_reliable) const noexcept -> bool {
  msgpack::sbuffer buffer;
  msgpack::packer packer{ &buffer };

  packer.pack_array(2);
  packer.pack(p_packet_type);
  packer.pack(p_obj);

  return send_raw(p_conn, buffer.data(), buffer.size(), p_reliable);
}

template <typename T>
auto Server::broadcast(const packet_type_t p_packet_type, const T& p_obj,
                       const bool p_reliable) noexcept -> bool {
  msgpack::sbuffer buffer;
  msgpack::packer packer{ &buffer };

  packer.pack_array(2);
  packer.pack(p_packet_type);
  packer.pack(p_obj);

  return broadcast_raw(buffer.data(), buffer.size(), p_reliable);
}

}  // namespace kb::net

#endif  //KB_NETWORKING_CPP_SERVER_H
