//
// Created by happy on 8/20/2025.
//

#ifndef KB_NETWORKING_UDP_PACKET_CLIENT_HPP
#define KB_NETWORKING_UDP_PACKET_CLIENT_HPP

#include "kb/net/udp/client/udp_client.hpp"
#include "kb/core/owning_buffer.hpp"
#include "kb/net/udp/packet.hpp"
#include "kb/net/udp/serde.hpp"

#include <msgpack.hpp>

namespace kb::net {

template <serialization_type_t SerdeT>
class UdpPacketClient : public std::enable_shared_from_this<UdpPacketClient<SerdeT>> {
public:
  using serde_traits = serde::serializer_traits<SerdeT>;
  // Underlying serialization handler
  using serializer_t = serde_traits::serializer_t;
  // Deserialized payload type
  using payload_t = serde_traits::payload_t;
  // Packet handler callback function type
  using packet_handler_func_t = std::function<void(const payload_t & p_payload)>;
public:

  [[nodiscard]] static auto create() -> std::shared_ptr<UdpPacketClient> {
    const auto client = std::make_shared<UdpPacketClient>();

    // TODO: consider moving weak-callback wrapping into UdpClient itself
    client->m_client.set_on_data_callback(
      [weak = client->weak_from_this()](incoming_view_message_t p_message) {
        if (auto self = weak.lock()) {
          self->handle_message(p_message);
        }
      }
    );

    return client;
  }

  auto connect(const std::string& p_address, const u16 p_port) noexcept -> bool {
    return m_client.connect(p_address, p_port);
  }

  auto stop() noexcept -> void { m_client.stop(); }

  template <typename T>
  auto send_packet(packet_type_t p_packet_type, const T & p_object, bool p_reliable = true) noexcept -> bool;

  auto bind_packet_handler(packet_type_t p_packet_type, packet_handler_func_t && p_handler) noexcept -> bool;
  auto unbind_packet_handler(packet_type_t p_packet_type) noexcept -> void;

  [[nodiscard]] auto get_connection_status() const noexcept -> UdpClient::connection_status_t {
    return m_client.get_connection_status();
  }

  [[nodiscard]] auto is_running() const noexcept -> bool { return m_client.is_running(); }

private:
  [[nodiscard]] auto send_packet_internal(const details::internal_packet_t & p_packet, bool p_reliable = true) const noexcept -> bool;
  [[nodiscard]] auto handle_message(incoming_view_message_t p_message) noexcept -> bool;

private:
  // Underlying udp client
  UdpClient m_client;
  // Registered packet handlers
  std::mutex m_handler_mutex; // TODO: do we need mutex ?
  std::unordered_map<packet_type_t, packet_handler_func_t> m_handlers;
  // Packet serialization handler
  serializer_t m_serializer{};
};

} // end namespace kb::net

#include "kb/net/udp/client/udp_packet_client.inl"

#endif  //KB_NETWORKING_UDP_PACKET_CLIENT_HPP
