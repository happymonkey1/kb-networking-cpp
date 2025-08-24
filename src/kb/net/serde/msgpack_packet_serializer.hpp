//
// Created by happy on 8/23/2025.
//

#ifndef KB_NETWORKING_MSGPACK_PACKET_SERIALIZER_HPP
#define KB_NETWORKING_MSGPACK_PACKET_SERIALIZER_HPP

#include "kb/net/serde/base_packet_serializer.hpp"

#include "kb/net/udp/serde.hpp"

namespace kb::net::serde {

class MsgpackPacketSerializer : public BasePacketSerializer<MsgpackPacketSerializer> {
public:
  using payload_t = msgpack::object_handle;

public:
  auto serialize_packet_internal(
    const kb_connection_t p_conn,
    packet_type_t p_packet_type,
    auto && p_object
  ) const noexcept -> details::internal_packet_t {
    auto header = details::internal_packet_header_t::create(
      serialization_type_t::msgpack,
      p_packet_type
    );

    // Serialize data with msgpack
    ::msgpack::sbuffer buffer{};
    ::msgpack::packer packer{ buffer };

    packer.pack(p_object);

    const auto write_size = buffer.size();
    const auto * data_ptr = reinterpret_cast<u8 *>(buffer.release());

    // COPY data into a new buffer with the internal packet header
    // TODO: optimize
    const auto packet_buffer_size = write_size + sizeof(details::internal_packet_header_t);
    auto * packet_buffer = new u8[packet_buffer_size];
    std::size_t cursor = MsgpackPacketSerializer::write_internal_packet_header(
      packet_buffer,
      header
    );
    std::memcpy(packet_buffer + cursor, data_ptr, write_size);

    KB_LOG_DEBUG("[MsgpackPacketSerializer]: Serialized packet with size: {} bytes", packet_buffer_size);

    return details::internal_packet_t{
      .m_header = header,
      .m_buffer = core::OwningBuffer::consume(&packet_buffer, packet_buffer_size),
      .m_conn = p_conn,
    };
  }

  [[nodiscard]] auto deserialize_packet_internal(
    const void * p_payload,
    const std::size_t p_payload_size
  ) const noexcept -> option<payload_t> {

    try {
      auto object_handle = msgpack::unpack(
        static_cast<const char *>(p_payload),
        p_payload_size
      );

#if 0
      if (object.type != msgpack::type::BIN) {
        KB_LOG_WARN(
          "[MsgpackPacketSerializer]: Deserialized msgpack payload but it is wrong type: {}",
          static_cast<std::underlying_type_t<msgpack::type::object_type>>(object.type)
        );
        return std::nullopt;
      }
#endif

      return std::make_optional(std::move(object_handle));
    } catch (std::exception & err) {
      KB_LOG_ERROR("[MsgpackPacketSerializer]: Failed to deserialize payload data: {}", err.what());
      return std::nullopt;
    }

  }

  template <typename T>
  [[nodiscard]] auto payload_as_internal(
    payload_t p_payload
  ) const noexcept -> option<T> {

    T temp{};
    try {
      auto object = p_payload.get();
      object.convert(temp);
    } catch (std::exception& err) {
      KB_LOG_ERROR("[MsgpackPacketSerializer]: Failed to deserialize payload data: {}", err.what());
      return std::nullopt;
    }

    return temp;
  }

};

} // end namespace kb::net


#endif  //KB_NETWORKING_MSGPACK_PACKET_SERIALIZER_HPP
