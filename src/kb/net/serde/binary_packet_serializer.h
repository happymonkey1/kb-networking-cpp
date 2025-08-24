//
// Created by happy on 8/23/2025.
//

#ifndef KB_NETWORKING_BINARY_PACKET_SERIALIZER_H
#define KB_NETWORKING_BINARY_PACKET_SERIALIZER_H

#include "kb/net/serde/base_packet_serializer.h"

#include <span>

namespace kb::net::serde {

class BinaryPacketSerializer : public BasePacketSerializer<BinaryPacketSerializer> {
public:
  using internal_packet_header_t = kb::net::details::internal_packet_header_t;
  using payload_t = std::span<const char>;
public:
  auto serialize_packet_internal(
    kb_connection_t p_conn,
    packet_type_t p_packet_type,
    auto && p_payload
  ) const noexcept -> details::internal_packet_t {
    static_assert(std::is_trivially_copyable_v<typename std::remove_cv<typename std::remove_reference<decltype(p_payload)>::type>::type>);

    return serialize(
      p_conn,
      &p_payload,
      sizeof(p_payload),
      p_packet_type
    );
  }

  [[nodiscard]] auto deserialize_packet_internal(
    const void * p_payload,
    const std::size_t p_payload_size
  ) const noexcept -> option<payload_t> {
    return std::make_optional(
      std::move(std::span{ static_cast<const char *>(p_payload), p_payload_size })
    );
  }

  [[nodiscard]] static auto serialize(
    const kb_connection_t   p_conn,
    const void            * p_data,
    const std::size_t       p_size,
    const packet_type_t     p_packet_type = static_cast<packet_type_t>(internal_packet_id_t::unknown)
  ) noexcept -> details::internal_packet_t {
    const auto data_size = sizeof(details::internal_packet_header_t) + p_size;

    // TODO: replace with arena allocator owned by client / server
    //       Arena allocator can use max size of last 3 ticks to pre-allocate buffer
    auto * buffer = new u8[data_size];

    std::size_t cursor = BinaryPacketSerializer::write_internal_packet_header(
      buffer,
      details::internal_packet_header_t::create(
        serialization_type_t::bin,
        static_cast<packet_type_t>(internal_packet_id_t::unknown)
      )
    );

    for (std::size_t i = 0; i < p_size; ++i) {
      buffer[cursor + i] = static_cast<u8*>(const_cast<void*>(p_data))[i];
    }

    return details::internal_packet_t{
      .m_header = {
        .m_serde_type = serialization_type_t::bin,
        .m_packet_type = p_packet_type,
      },
      .m_buffer = core::OwningBuffer::consume(&buffer, data_size),
      .m_conn = p_conn,
    };
  }
};

} // end namespace kb::net

#endif  //KB_NETWORKING_BINARY_PACKET_SERIALIZER_H
