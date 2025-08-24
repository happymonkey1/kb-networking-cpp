//
// Created by happymonkey1 on 8/17/25.
//

#ifndef KB_NETWORKING_PACKET_H
#define KB_NETWORKING_PACKET_H

#include "kb/core/logger.hpp"
#include "kb/kb_networking.h"
#include "kb/core/owning_buffer.hpp"
#include "kb/types.h"
#include "kb/net/network_types.h"
#include <msgpack.hpp>

namespace kb::net {

constexpr u8 k_magic_byte_0 = KB_MAGIC_BYTE_0;
constexpr u8 k_magic_byte_1 = KB_MAGIC_BYTE_1;
constexpr u8 k_magic_byte_2 = KB_MAGIC_BYTE_2;
// Magic bytes header: KBL
constexpr char k_magic_bytes[3] = {
  k_magic_byte_0,
  k_magic_byte_1,
  k_magic_byte_2,
};

namespace details {

// TODO: consider optimizing to 4 bytes
//       TWO bytes header: 'K' 'B'
//       TWO bytes for serde and packet type
//         4 bits for serialization type (16 possible serde types)
//         12 bits for packet type (4096 possible packets)
//       packet_type_t can be changed to uint16_t
struct internal_packet_header_t {
  // Kablunk magic header
  u8                    m_magic_header[sizeof(k_magic_bytes)] = {
    k_magic_byte_0,
    k_magic_byte_1,
    k_magic_byte_2,
  };
  // Serialization format of the serialized data
  serialization_type_t  m_serde_type = serialization_type_t::bin;
  // Packet serializer_t (ID) used for handler dispatching
  packet_type_t         m_packet_type = static_cast<packet_type_t>(internal_packet_id_t::unknown);

  static auto create(serialization_type_t p_serde_type, packet_type_t p_packet_type) noexcept -> internal_packet_header_t {
    internal_packet_header_t header{};
    header.m_serde_type  = p_serde_type;
    header.m_packet_type = p_packet_type;
    return header;
  }
};

struct internal_packet_t {
  // Internal packet header
  internal_packet_header_t m_header;
  // Owning buffer to serialized data
  core::OwningBuffer       m_buffer;
  // Packet connection handle
  kb_connection_t          m_conn;

  // Create a packet from a header and pre-serialized data
  // Packet takes ownership of the data (and it's lifetime)
  static auto create(
    details::internal_packet_header_t p_header,
    void                              ** p_data,
    const std::size_t                    p_size
    ) noexcept -> details::internal_packet_t {
    auto buffer = core::OwningBuffer::consume(
      reinterpret_cast<core::OwningBuffer::value_t **>(p_data),
      p_size
    );

    return details::internal_packet_t{
      .m_header = std::move(p_header),
      .m_buffer = std::move(buffer),
    };
  }
};

[[nodiscard]] inline auto unpack_internal_packet_header(
  const void * p_data,
  const std::size_t p_size
) noexcept -> option<internal_packet_header_t> {
  constexpr auto k_header_size = sizeof(internal_packet_header_t);
  if (p_size < k_header_size) {
    KB_LOG_ERROR(
      "[Packet] Can not check for reserved packet with insufficient data size: {} < {}",
      p_size,
      k_header_size
    );
    return std::nullopt;
  }

  const auto * data_buffer = static_cast<const u8 *>(p_data);
  std::size_t cursor = 0;
  // Check for reserved packet magic bytes
  for (; cursor < sizeof(k_magic_bytes); ++cursor) {
    const auto magic_byte = k_magic_bytes[cursor];
    // TODO: this is somewhat brittle serializer_t casting
    if (reinterpret_cast<typeof(magic_byte) *>(data_buffer)[cursor] != magic_byte) {
      return std::nullopt;
    }
  }

  // Unpack header data
  const auto serialization_type = static_cast<serialization_type_t>(data_buffer[cursor++]);
  packet_type_t packet_type;
  std::memcpy(&packet_type, data_buffer + cursor, sizeof(packet_type_t));
  cursor += sizeof(packet_type_t);

  KB_ASSERT(
    cursor == sizeof(internal_packet_header_t),
    "Unexpected cursor position during packet header unpacking: Expected %d, found %d.",
    sizeof(internal_packet_header_t),
    cursor
  );

  return std::make_optional(internal_packet_header_t{
    .m_serde_type = serialization_type,
    .m_packet_type = packet_type,
  });
}

} // end namespace ::details

} // end namespace kb::net

#endif  //KB_NETWORKING_PACKET_H
