//
// Created by happymonkey1 on 8/16/25.
//

#ifndef KB_NETWORKING_CPP_SERVER_H
#define KB_NETWORKING_CPP_SERVER_H

#include "kb/types.h"

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>

#include <functional>
#ifndef STEAMNETWORKINGSOCKETS_OPENSOURCE
#include <steam/steam_api.h>
#endif

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

public:
  Server();
  ~Server() noexcept;

  auto start_async(u16 p_port) noexcept -> bool;
  auto start_manual(u16 p_port) noexcept -> bool;
  auto stop() noexcept -> void;

  auto poll() noexcept -> void;

  auto send(HSteamNetConnection p_conn, const void *p_data, size_t p_len, bool p_reliable) const noexcept -> bool;
  auto broadcast(const void *p_data, size_t p_len, bool p_reliable) noexcept -> bool;
  auto disconnect(HSteamNetConnection p_conn, i32 p_reason = 0) noexcept;

  auto is_running() const noexcept -> bool { return m_is_running.load(); }
  auto port() const noexcept -> u16 { return m_port; }

  auto set_on_data_callback(const data_received_callback_func_t& p_call) noexcept -> void {
    m_data_received_callback = p_call;
  }

  auto set_on_client_connected_callback(const client_connected_callback_func_t& p_call) noexcept -> void {
    m_client_connected_callback = p_call;
  }

  auto set_on_client_disconnected_callback(const client_disconnected_callback_func_t& p_call) noexcept -> void {
    m_client_disconnected_callback = p_call;
  }

  auto get_client_info(HSteamNetConnection p_conn) noexcept -> const client_info_t * {
    std::lock_guard lock{ m_client_mutex };
    const auto client = m_clients.find(p_conn);
    return client != m_clients.end() ? &client->second : nullptr;
  }

private:
  auto network_loop() noexcept -> void;
  static auto connection_status_changed_callback(SteamNetConnectionStatusChangedCallback_t *p_info) noexcept -> void;
  auto on_connection_status_changed(const SteamNetConnectionStatusChangedCallback_t *p_info) noexcept -> void;

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

  ISteamNetworkingSockets *m_interface = nullptr;
  HSteamListenSocket m_listen_socket = k_HSteamListenSocket_Invalid;
  HSteamNetPollGroup m_poll_group = k_HSteamNetPollGroup_Invalid;
};

}

#endif  //KB_NETWORKING_CPP_SERVER_H
