//
// Created by happymonkey1 on 8/17/25.
//

#include "server.hpp"

#include <ranges>

#include "kb/kb_networking_cpp.hpp"

namespace kb::net {

static Server * s_instance = nullptr;

Server::Server() {
  m_interface = SteamNetworkingSockets();
}

Server::~Server() noexcept {
  stop();
}

auto Server::start_async(const u16 p_port) noexcept -> bool {
  if (!start_manual(p_port)) {
    return false;
  }

  m_network_thread = std::thread([this] { network_loop(); });

  return true;
}

auto Server::start_manual(const u16 p_port) noexcept -> bool {
  if (m_is_running.load()) {
    KB_LOG_WARN("Server is already running");
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

  KB_LOG_TRACE("Server listening on port: {}", p_port);

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

auto Server::stop() noexcept -> void {
  s_instance = nullptr;
  if (!m_is_running.exchange(false)) {
    return;
  }

  if (m_network_thread.joinable()) {
    m_network_thread.join();
  }

  std::lock_guard lock(m_client_mutex);
  KB_LOG_TRACE("Server stopped");
  for (const auto& conn : m_clients | std::views::keys) {
    m_interface->CloseConnection(conn, 0, "Server shutting down", true);
  }
  m_clients.clear();
  KB_LOG_TRACE("Disconnected all clients");

  if (m_listen_socket != k_HSteamListenSocket_Invalid) {
    m_interface->CloseListenSocket(m_listen_socket);
    m_listen_socket = k_HSteamListenSocket_Invalid;
  }

  if (m_poll_group != k_HSteamNetPollGroup_Invalid) {
    m_interface->DestroyPollGroup(m_poll_group);
    m_poll_group = k_HSteamNetPollGroup_Invalid;
  }
}
auto Server::poll() noexcept -> void {
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

    // Try handle packet with registered handler
    const auto handle_packet_res = handle_packet(conn, message->m_pData, message->m_cbSize);

    if (!handle_packet_res && m_data_received_callback) {
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

auto Server::send_raw(const HSteamNetConnection p_conn, const void* p_data, const size_t p_len,
                  const bool p_reliable) const noexcept -> bool {
  const int flags = p_reliable ? k_nSteamNetworkingSend_Reliable : k_nSteamNetworkingSend_Unreliable;
  return m_interface->SendMessageToConnection(
    p_conn,
    p_data,
    static_cast<u32>(p_len),
    flags,
    nullptr
  );
}

auto Server::broadcast_raw(const void* p_data, const size_t p_len,
                       const bool p_reliable) noexcept -> bool {
  std::lock_guard lock{ m_client_mutex };
  bool ok = true;
  for (auto& conn : m_clients | std::views::keys) {
    if (!send_raw(conn, p_data, p_len, p_reliable)) {
      ok = false;
      KB_LOG_ERROR("Failed to send data to client: {}", conn);
    }
  }

  return ok;
}

auto Server::disconnect(const HSteamNetConnection p_conn, const i32 p_reason) noexcept -> void {
  m_interface->CloseConnection(p_conn, p_reason, "Disconnected", false);
  std::lock_guard lock{ m_client_mutex };
  m_clients.erase(p_conn);
}
auto Server::bind_packet_handler(
    packet_type_t p_packet_type,
    packet_handler_func_t&& packet_handler_func) noexcept -> bool {
  if (m_packet_handlers.contains(p_packet_type)) {
    KB_LOG_ERROR("Failed to bind packet handler for packet type: {}. It is already bound!", p_packet_type);
    return false;
  }

  m_packet_handlers[p_packet_type] = packet_handler_func;
  KB_LOG_DEBUG("Successfully bound packet handler for packet: {}", p_packet_type);
  return true;
}

auto Server::network_loop() noexcept -> void {
  while (m_is_running.load()) {
    poll();
    std::this_thread::sleep_for(std::chrono::milliseconds(k_default_poll_timeout));
  }
}

auto Server::connection_status_changed_callback(
    SteamNetConnectionStatusChangedCallback_t *p_info) noexcept -> void {
  if (!s_instance) {
    KB_LOG_ERROR("Failed to call connection status changed callback because Server instance is not set");
    return;
  }

  s_instance->on_connection_status_changed(p_info);
}

auto Server::on_connection_status_changed(
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
        KB_LOG_WARN("Failed to accept connection '{}' (it was already closed?)", conn);
        break;
      }

      if (!m_interface->SetConnectionPollGroup(conn, m_poll_group)) {
        m_interface->CloseConnection(conn, 0, nullptr, false);
        KB_LOG_WARN("Failed to set connection poll group for connection: {}", conn);
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

auto Server::handle_packet(const HSteamNetConnection p_conn, const void* p_data,
                           const size_t p_len) noexcept -> bool {
  try {
    const msgpack::object_handle object_handle = msgpack::unpack(static_cast<const char*>(p_data), p_len);
    const msgpack::object object = object_handle.get();

    if (object.type != msgpack::type::ARRAY || object.via.array.size != 2) {
      KB_LOG_ERROR("Invalid packet type");
      return false;
    }

    const auto packet_type = object.via.array.ptr[0].as<packet_type_t>();
    const msgpack::object payload = object.via.array.ptr[1].as<msgpack::object>();

    if (const auto packet_handler_func = m_packet_handlers.find(packet_type);
        packet_handler_func != m_packet_handlers.end()) {
      KB_LOG_DEBUG("Handling packet type: {}", packet_type);
      packet_handler_func->second(p_conn, payload);
    } else {
      KB_LOG_ERROR("Received valid packet but failed to retrieve any packet handler");
      return false;
    }

    return true;
  } catch (const std::exception& e) {
    KB_LOG_ERROR("Failed to handle packet with msgpack: {}", e.what());
    return false;
  }
}


auto Server::on_fatal_message(const char* p_msg) noexcept -> void {
  if (p_msg) {
    KB_LOG_ERROR(p_msg);
  }

  m_is_running.store(false);
}

}
