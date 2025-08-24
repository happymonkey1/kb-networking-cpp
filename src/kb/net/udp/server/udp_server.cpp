//
// Created by happymonkey1 on 8/17/25.
//

#include "udp_server.hpp"
#include "kb/kb_networking_cpp.hpp"

#include <steam/isteamnetworkingutils.h>
#include <steam/steamnetworkingsockets.h>
#include <steam/steamnetworkingtypes.h>

#include <ranges>


namespace kb::net {

static UdpServer* s_instance = nullptr;

UdpServer::UdpServer() {
  m_interface = SteamNetworkingSockets();
}

UdpServer::~UdpServer() noexcept {
  stop();
}

auto UdpServer::start_async(const u16 p_port) noexcept -> bool {
  if (!start_manual(p_port)) {
    return false;
  }

  m_network_thread = std::thread([this] { network_loop(); });

  return true;
}

auto UdpServer::start_manual(const u16 p_port) noexcept -> bool {
  if (m_is_running.load()) {
    KB_LOG_WARN("[UdpServer] UdpServer is already running");
    return false;
  }

  SteamNetworkingIPAddr server_addr;
  server_addr.Clear();
  server_addr.m_port = p_port;

  // Configure options for listen socket
  // k_n_opts should equal the number of SetPtr calls
  SteamNetworkingConfigValue_t opts;
  constexpr u32 k_n_opts = 1;
  opts.SetPtr(
    k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged,
    reinterpret_cast<void*>(connection_status_changed_callback)
  );

  m_listen_socket = m_interface->CreateListenSocketIP(server_addr, k_n_opts, &opts);

  if (m_listen_socket == k_HSteamListenSocket_Invalid) {
    on_fatal_message("Failed to initialize listen socket");
    return false;
  }

  KB_LOG_TRACE("[UdpServer] UdpServer listening on port: {}", p_port);

  m_poll_group = m_interface->CreatePollGroup();
  if (!m_poll_group) {
    on_fatal_message("Failed to create poll group");
    return false;
  }

  s_instance = this;
  m_port = p_port;
  m_is_running.store(true);
  return true;
}

auto UdpServer::stop() noexcept -> void {
  s_instance = nullptr;
  if (!m_is_running.exchange(false)) {
    return;
  }

  if (m_network_thread.joinable()) {
    m_network_thread.join();
  }

  std::scoped_lock lock(m_client_mutex);
  KB_LOG_TRACE("[UdpServer] UdpServer stopped");
  for (const auto& conn : m_clients | std::views::keys) {
    m_interface->CloseConnection(conn, 0, "UdpServer shutting down", true);
  }
  m_clients.clear();
  KB_LOG_TRACE("[UdpServer] Disconnected all clients");

  if (m_listen_socket != k_HSteamListenSocket_Invalid) {
    m_interface->CloseListenSocket(m_listen_socket);
    m_listen_socket = k_HSteamListenSocket_Invalid;
  }

  if (m_poll_group != k_HSteamNetPollGroup_Invalid) {
    m_interface->DestroyPollGroup(m_poll_group);
    m_poll_group = k_HSteamNetPollGroup_Invalid;
  }
}
auto UdpServer::poll() noexcept -> void {
  constexpr u32 k_message_count = 32;
  ISteamNetworkingMessage *messages[k_message_count];
  const u32 message_count = m_interface->ReceiveMessagesOnPollGroup(
    m_poll_group,
    messages,
    k_message_count
  );

  for (u32 i = 0; i < message_count; ++i) {
    auto *message = messages[i];
    const auto conn = message->m_conn;

    if (m_data_received_callback) {
      // Otherwise, fallback to generate data received callback
      if (const auto* client = get_client_info(conn); client) {
        m_data_received_callback(
          *client,
          message->m_pData,
          message->m_cbSize
        );
      }
    }
    message->Release();
  }

  m_interface->RunCallbacks();
}

auto UdpServer::send(const kb_connection_t p_conn, const void * p_data, const u32 p_size, const bool p_reliable) const noexcept -> bool {
  const int flags = p_reliable ? k_nSteamNetworkingSend_Reliable : k_nSteamNetworkingSend_Unreliable;
  const auto result = m_interface->SendMessageToConnection(
    p_conn,
    p_data,
    p_size,
    flags,
    nullptr
  );

  return result == k_EResultOK;
}

auto UdpServer::broadcast(const void * p_data, u32 p_size, bool p_reliable) noexcept -> bool {
  std::scoped_lock lock{ m_client_mutex };
  if (m_clients.empty()) {
    return false;
  }

  bool ok = true;
  for (auto& conn : m_clients | std::views::keys) {
    if (!send(conn, p_data, p_size, p_reliable)) {
      ok = false;
      KB_LOG_ERROR("[UdpServer] Failed to send data to client: {}", conn);
    }
  }

  return ok;
}

auto UdpServer::disconnect(const kb_connection_t p_conn, const i32 p_reason) noexcept -> void {
  m_interface->CloseConnection(p_conn, p_reason, "Disconnected", false);
  std::scoped_lock lock{ m_client_mutex };
  m_clients.erase(p_conn);
}

auto UdpServer::network_loop() noexcept -> void {
  while (m_is_running.load()) {
    poll();
    std::this_thread::sleep_for(std::chrono::milliseconds(k_default_poll_timeout));
  }
}

auto UdpServer::connection_status_changed_callback(
    SteamNetConnectionStatusChangedCallback_t *p_info) noexcept -> void {
  if (!s_instance) {
    KB_LOG_ERROR("[UdpServer] Failed to call connection status changed callback because UdpServer instance is not set");
    return;
  }

  s_instance->on_connection_status_changed(p_info);
}

auto UdpServer::on_connection_status_changed(
    const SteamNetConnectionStatusChangedCallback_t *p_info) noexcept -> void {
  switch (p_info->m_info.m_eState) {
    case k_ESteamNetworkingConnectionState_None:
      // Callback after we destroy connections. For now, these are ignored.
      break;

    case k_ESteamNetworkingConnectionState_ClosedByPeer: [[fallthrough]];
    case k_ESteamNetworkingConnectionState_ProblemDetectedLocally: {
      const auto conn = p_info->m_hConn;

      // Notify client disconnected callback
      if (p_info->m_eOldState == k_ESteamNetworkingConnectionState_Connected) {
        if (const auto* client_info = get_client_info(conn);
            m_client_disconnected_callback && client_info) {
          m_client_disconnected_callback(*client_info);
        }
      }

      // Clean up the connection because it has been "closed" in a network sense,
      // but not destroyed. We need to close on our end too.
      m_interface->CloseConnection(conn, 0, nullptr, false);
      break;
    }

    case k_ESteamNetworkingConnectionState_Connecting: {
      const auto conn = p_info->m_hConn;
      if (m_interface->AcceptConnection(conn) != k_EResultOK) {
        m_interface->CloseConnection(conn, 0, nullptr, false);
        KB_LOG_WARN("[UdpServer] Failed to accept connection '{}' (it was already closed?)", conn);
        break;
      }

      if (!m_interface->SetConnectionPollGroup(conn, m_poll_group)) {
        m_interface->CloseConnection(conn, 0, nullptr, false);
        KB_LOG_WARN("[UdpServer] Failed to set connection poll group for connection: {}", conn);
        break;
      }

      SteamNetConnectionInfo_t conn_info;
      m_interface->GetConnectionInfo(conn, &conn_info);

      auto& client = m_clients[conn];
      client.m_conn = conn;
      client.m_addr = conn_info.m_addrRemote;

      if (m_client_connected_callback) {
        m_client_connected_callback(client);
      }

      break;
    }

    case k_ESteamNetworkingConnectionState_Connected: {
      // The server can ignore connected status
      break;
    }
  }
}

auto UdpServer::on_fatal_message(const char* p_msg) noexcept -> void {
  if (p_msg) {
    KB_LOG_ERROR(p_msg);
  }

  m_is_running.store(false);
}

}
