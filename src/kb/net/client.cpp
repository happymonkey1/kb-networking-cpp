#include "client.hpp"

#include "network_utils.h"

namespace kb::net {

static Client * s_instance = nullptr;

Client::~Client() noexcept {
  stop();
}

auto Client::stop() noexcept -> void {
  s_instance = nullptr;
  if (!m_running.exchange(false)) {
    return;
  }

  if (m_network_thread.joinable()) {
    m_network_thread.join();
  }

  if (m_conn != k_HSteamNetConnection_Invalid) {
    m_interface->CloseConnection(m_conn, 0, "Client disconnecting", true);
    m_conn = k_HSteamNetConnection_Invalid;
  }

  KB_LOG_DEBUG("Finished destroying client");
}

auto Client::poll() noexcept -> void {
  if (!m_running.load()) {
    return;
  }

  poll_messages();
  process_incoming_messages();

  if (m_interface) {
    m_interface->RunCallbacks();
  }
}
auto Client::connect(const std::string& p_address, u16 p_port) noexcept
    -> bool {
  if (m_running.load()) {
    return false;
  }

  const auto res = try_connect(p_address, p_port);
  m_running.store(res);

  m_network_thread = std::thread([this] { network_loop(); });

  return res;
}

auto Client::async_connect(const std::string& p_address, u16 p_port) noexcept
    -> coro::task<bool> {
  co_await m_scheduler->schedule();

  if (m_running.load()) {
    KB_LOG_WARN("[async_connect] client is already running");
    co_return false;
  }

  const auto res = try_connect(p_address, p_port);
  m_running.store(res);
  if (!res) {
    KB_LOG_ERROR("[async_connect] Failed to connect to server, returning early");
    co_return false;
  }

  m_network_thread = std::thread([this] { network_loop(); });

  constexpr u32 k_max_wait_ms = 1000;
  constexpr u32 k_sleep_duration = 50;
  constexpr u32 max_iters = k_max_wait_ms / k_sleep_duration;
  u32 iter = 0;
  while (m_connection_status == connection_status_t::connecting) {
    if (iter++ >= max_iters) {
      KB_LOG_WARN("[async_connect] Failed to connect to server after {} ms", k_sleep_duration);
      break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  KB_LOG_INFO("[Client] Successfully connected to server: {}:{}", p_address, p_port);
  co_return m_connection_status == connection_status_t::connected;
}

auto Client::try_receive_raw(msgpack::object_handle& p_out_object,
                             HSteamNetConnection& p_out_conn) noexcept -> void {
  std::scoped_lock lock{ m_message_queue_mutex };
  if (m_message_queue.empty()) {
    return;
  }

  const auto message = m_message_queue.back();
  m_message_queue.pop_back();
  p_out_conn = message.m_conn;
  p_out_object = msgpack::unpack(message.m_data.data(), message.m_data.size());
}

auto Client::send_raw(const void* p_data, const size_t p_len,
                      const bool p_reliable) const noexcept -> bool {
  if (!m_interface || m_conn == k_HSteamNetConnection_Invalid) {
    return false;
  }

  const int flags = p_reliable ? k_nSteamNetworkingSend_Reliable : k_nSteamNetworkingSend_Unreliable;
  const auto res = m_interface->SendMessageToConnection(m_conn, p_data, p_len, flags, nullptr);
  return res == k_EResultOK;
}

auto Client::bind_handler(packet_type_t p_packet_type,
                          packet_handler_func_t&& p_handler) noexcept -> bool {
  std::scoped_lock lock{ m_handler_mutex };
  if (m_handlers.contains(p_packet_type)) {
    return false;
  }

  m_handlers.emplace(p_packet_type, std::move(p_handler));
  return true;
}

auto Client::unbind_handler(packet_type_t p_packet_type) noexcept -> void {
  std::scoped_lock lock{ m_handler_mutex };
  m_handlers.erase(p_packet_type);
}

auto Client::poll_messages() noexcept -> void {
  if (!m_interface || m_conn == k_HSteamNetConnection_Invalid) {
    KB_LOG_ERROR("Failed to poll for messages because interface or connection is invalid!");
    return;
  }

  ISteamNetworkingMessage *messages[k_default_poll_message_count];
  const i32 message_count = m_interface->ReceiveMessagesOnConnection(m_conn, messages, k_default_poll_message_count);
  for (i32 i = 0; i < message_count; ++i) {
    auto *message = messages[i];
    push_message_to_queue(message->m_conn, message->m_pData, message->m_cbSize);
    message->Release();
  }
}

auto Client::push_message_to_queue(const HSteamNetConnection p_conn,
                                   const void* p_data,
                                   const size_t p_len) noexcept -> void {
  std::scoped_lock lock{ m_message_queue_mutex };
  m_message_queue.emplace_back(
    incoming_message_t{
      .m_conn = p_conn,
      .m_data = std::vector(
        static_cast<const char *>(p_data),
        static_cast<const char *>(p_data) + p_len
      )
    }
  );
}

auto Client::network_loop() noexcept -> void {
  KB_LOG_INFO("[Client] Entered network loop");
  while (m_running.load()) {
    poll();
    std::this_thread::sleep_for(std::chrono::milliseconds(k_default_poll_delay_ms));
  }
}

auto Client::register_packet_awaiter(packet_type_t p_packet_type,
                                     awaiting_packet_t p_awaiting) noexcept
    -> void {
  std::scoped_lock lock{ m_awaiters_mutex };
  if (m_awaiters.contains(p_packet_type)) {
    auto& queue = m_awaiters[p_packet_type];
    queue.push(std::move(p_awaiting));
  } else {
    std::queue<awaiting_packet_t> queue;
    queue.push(std::move(p_awaiting));
    m_awaiters.emplace(
      p_packet_type,
      std::move(queue)
    );
  }
}

auto Client::get_packet_awaiter(packet_type_t p_packet_type) noexcept -> std::optional<awaiting_packet_t> {
  std::scoped_lock lock{ m_awaiters_mutex };
  auto& queue = m_awaiters[p_packet_type];
  if (queue.empty()) {
    return std::nullopt;
  }

  const auto awaiting = std::make_optional(std::move(queue.front()));
  queue.pop();
  return std::move(awaiting);
}

auto Client::connection_status_changed_callback(
    SteamNetConnectionStatusChangedCallback_t *p_info) noexcept -> void {
  if (!s_instance) {
    KB_LOG_ERROR("Can not invoke connection status callback because client instance is not set");
    return;
  }

  s_instance->on_connection_status_changed(p_info);
}
auto Client::on_connection_status_changed(
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

auto Client::async_wait_for_packet(packet_type_t p_packet_type) noexcept
    -> coro::task<std::optional<msgpack::object>> {
  // Immediately schedule on the executor
  co_await m_scheduler->schedule();
  coro::event event;
  register_packet_awaiter(p_packet_type, {
      .m_event = &event,
      .m_data = std::nullopt,
  });
  co_await event;
  auto awaiter = get_packet_awaiter(p_packet_type);
  if (!awaiter.has_value() || !awaiter->m_data) {
    co_return std::nullopt;
  }

  co_return std::optional(std::move(*awaiter->m_data));
}

auto Client::try_connect(const std::string& p_address, u16 p_port) noexcept
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

auto Client::process_incoming_messages() noexcept -> void {
  std::vector<incoming_message_t> local_messages;
  {
    std::scoped_lock lock{ m_message_queue_mutex };
    local_messages.swap(m_message_queue);
  }

  for (auto& message : local_messages) {
    try {
      const auto object_handle = msgpack::unpack(message.m_data.data(), message.m_data.size());
      const auto object = object_handle.get();

      if (object.type != msgpack::type::ARRAY || object.via.array.size != 2) {
        KB_LOG_ERROR("Client received malformed packet");
        continue;
      }

      const auto packet_type = static_cast<packet_type_t>(
        object.via.array.ptr[0].as<u32>()
      );
      msgpack::object payload = object.via.array.ptr[1];

      // Check if we have any awaiters
      {
        std::scoped_lock lock{ m_message_queue_mutex };
        auto awaiters_it = m_awaiters.find(packet_type);
        if (awaiters_it != m_awaiters.end() && !awaiters_it->second.empty()) {
          auto& oldest = awaiters_it->second.front();
          oldest.m_data = std::make_optional(payload);
          oldest.m_event->set();
          continue;
        }
      }

      // Fallback to a registered packet handler
      packet_handler_func_t packet_handler_func;
      {
        std::scoped_lock lock{ m_handler_mutex };
        if (const auto it = m_handlers.find(packet_type);
            it != m_handlers.end()) {
          packet_handler_func = it->second;
        }
      }

      if (packet_handler_func) {
        packet_handler_func(payload);
      } else {
        KB_LOG_ERROR(
          "Client received valid packet {} but there is no assigned handler!",
          static_cast<u32>(packet_type)
        );
      }
    } catch (const std::exception& e) {
      KB_LOG_ERROR("Failed to unpack message with msgpack: {}", e.what());
    }
  }
}

} // end namespace kb::net
