#include "udp_client.hpp"

#include "kb/net/network_utils.h"

#include <steam/steamnetworkingtypes.h>
#include <steam/isteamnetworkingutils.h>
#include <steam/steamnetworkingsockets.h>

namespace kb::net {

static UdpClient* s_instance = nullptr;

UdpClient::~UdpClient() noexcept {
  stop();
}

auto UdpClient::stop() noexcept -> void {
  s_instance = nullptr;
  if (!m_running.exchange(false)) {
    return;
  }

  if (m_network_thread.joinable()) {
    m_network_thread.join();
  }

  if (m_conn != k_HSteamNetConnection_Invalid) {
    m_interface->CloseConnection(m_conn, 0, "UdpClient disconnecting", true);
    m_conn = k_HSteamNetConnection_Invalid;
  }

  KB_LOG_DEBUG("Finished destroying client");
}

auto UdpClient::poll() noexcept -> void {
  if (!m_running.load()) {
    return;
  }

  poll_messages();

  if (m_interface) {
    m_interface->RunCallbacks();
  }
}
auto UdpClient::connect(const std::string& p_address, u16 p_port) noexcept
    -> bool {
  if (m_running.load()) {
    return false;
  }

  const auto res = try_connect(p_address, p_port);
  m_running.store(res);

  m_network_thread = std::thread([this] { network_loop(); });

  return res;
}

auto UdpClient::try_connect(const std::string& p_address, u16 p_port) noexcept
  -> bool {
  m_connection_status = connection_status_t::connecting;
  m_interface = SteamNetworkingSockets();
  s_instance = this;

  const auto address_with_port = fmt::format("{}:{}", p_address, p_port);
  if (!utils::is_valid_ip_address(address_with_port)) {
    KB_LOG_WARN("Can not connect to invalid ip address: {}", address_with_port);
    m_connection_status = connection_status_t::failed_to_connect;
    return false;
  }

  SteamNetworkingIPAddr server_address;
  server_address.ParseString(address_with_port.c_str());

  SteamNetworkingConfigValue_t options;
  options.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void *)&connection_status_changed_callback);
  m_conn = m_interface->ConnectByIPAddress(server_address, 1, &options);
  if (m_conn == k_HSteamNetConnection_Invalid) {
    m_connection_status = connection_status_t::failed_to_connect;
    KB_LOG_ERROR("Failed to connect to host");
    return false;
  }

  return true;
}

auto UdpClient::send_raw(const void* p_data, const size_t p_len,
                      const bool p_reliable) const noexcept -> bool {
  if (!m_interface || m_conn == k_HSteamNetConnection_Invalid) {
    return false;
  }

  const int flags = p_reliable ? k_nSteamNetworkingSend_Reliable : k_nSteamNetworkingSend_Unreliable;
  const auto res = m_interface->SendMessageToConnection(m_conn, p_data, p_len, flags, nullptr);
  return res == k_EResultOK;
}

auto UdpClient::poll_messages() noexcept -> void {
  if (!m_interface || m_conn == k_HSteamNetConnection_Invalid) {
    KB_LOG_ERROR("Failed to poll for messages because interface or connection is invalid!");
    return;
  }

  ISteamNetworkingMessage *messages[k_default_poll_message_count];
  const i32 message_count = m_interface->ReceiveMessagesOnConnection(m_conn, messages, k_default_poll_message_count);

  if (m_callbacks.m_on_data_received) {
    for (std::size_t i = 0; i < message_count; ++i) {
      const auto * steam_message = messages[i];

      incoming_view_message_t message{
        .m_conn = m_conn,
        .m_buffer = {
          static_cast<const char *>(steam_message->m_pData),
          // TODO: handle negative values, probably error ?
          static_cast<u32>(steam_message->m_cbSize)
        }
      };

      m_callbacks.m_on_data_received(message);
    }
  }

  for (size_t i = 0; i < message_count; ++i) {
    messages[i]->Release();
  }
}

auto UdpClient::network_loop() noexcept -> void {
  KB_LOG_INFO("[UdpClient] Entered network loop");
  while (m_running.load()) {
    poll();
    std::this_thread::sleep_for(std::chrono::milliseconds(k_default_poll_delay_ms));
  }
}

auto UdpClient::connection_status_changed_callback(
    SteamNetConnectionStatusChangedCallback_t *p_info) noexcept -> void {
  if (!s_instance) {
    KB_LOG_ERROR("Can not invoke connection status callback because client instance is not set");
    return;
  }

  s_instance->on_connection_status_changed(p_info);
}
auto UdpClient::on_connection_status_changed(
    SteamNetConnectionStatusChangedCallback_t *p_info) noexcept -> void {
  switch (p_info->m_info.m_eState) {
    case k_ESteamNetworkingConnectionState_None:
      // Callback when connections are destroyed. We ignore these.
      break;

    case k_ESteamNetworkingConnectionState_ClosedByPeer: [[fallthrough]];
    case k_ESteamNetworkingConnectionState_ProblemDetectedLocally: {
      m_running.store(false);
      m_connection_status = connection_status_t::failed_to_connect;

      const char *error_message = p_info->m_info.m_szEndDebug;
      if (p_info->m_eOldState == k_ESteamNetworkingConnectionState_Connecting) {
        KB_LOG_WARN("Failed to connect to remote host: {}", error_message);
      } else if (p_info->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally) {
        KB_LOG_WARN("Lost connection with remote host: {}", error_message);
      } else {
        KB_LOG_WARN("Disconnected from host: {}", error_message);
      }

      m_interface->CloseConnection(p_info->m_hConn, 0, nullptr, false);
      m_conn = k_HSteamNetConnection_Invalid;
      m_connection_status = connection_status_t::disconnected;

      break;
    }

    case k_ESteamNetworkingConnectionState_Connecting:
      m_connection_status = connection_status_t::connecting;
      break;

    case k_ESteamNetworkingConnectionState_Connected: {
      m_connection_status = connection_status_t::connected;
      // TODO: callback
      break;
    }
  }
}

} // end namespace kb::net
