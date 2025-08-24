//
// Created by happy on 8/23/2025.
//

#ifndef KB_NETWORKING_BASE_PACKET_SERIALIZER_H
#define KB_NETWORKING_BASE_PACKET_SERIALIZER_H

#include "kb/net/network_types.h"
#include "kb/net/udp/packet.hpp"

namespace kb::net::serde {

template <typename DerivedT>
struct BasePacketSerializer {

  [[nodiscard]] auto serialize_packet(
    const kb_connection_t    p_conn,
    packet_type_t            p_packet_type,
    auto                  && p_obj
  ) const noexcept -> details::internal_packet_t {
    return derived()->serialize_packet_internal(p_conn, p_packet_type, std::forward<decltype(p_obj)>(p_obj));
  }

  [[nodiscard]] auto deserialize_packet(
    const void * p_payload,
    const std::size_t p_payload_size
  ) const noexcept -> auto {
    return derived()->deserialize_packet_internal(p_payload, p_payload_size);
  }
protected:
  [[nodiscard]] static auto write_internal_packet_header(
    u8 * p_buffer,
    details::internal_packet_header_t p_header
  ) noexcept -> std::size_t {
    std::size_t cursor = 0;

    // Emplace magic header
    for (; cursor < sizeof(k_magic_bytes); ++cursor) {
      p_buffer[cursor] = k_magic_bytes[cursor];
    }

    // Emplace serialization serializer_t
    static_assert(sizeof(std::underlying_type_t<serialization_type_t>) == 1);
    p_buffer[cursor++] = static_cast<u8>(p_header.m_serde_type);

    // Emplace packet serializer_t
    constexpr auto packet_type_size = sizeof(packet_type_t);
    std::memcpy(p_buffer + cursor, &p_header.m_packet_type, packet_type_size);
    cursor += packet_type_size;

    KB_ASSERT(
      cursor == sizeof(details::internal_packet_header_t),
      "Expected cursor to be at %d, found %d instead",
      sizeof(details::internal_packet_header_t),
      cursor
    );

    return cursor;
  }

private:
  [[nodiscard]] auto derived() const noexcept -> const DerivedT * { return static_cast<const DerivedT *>(this); }
  [[nodiscard]] auto derived() noexcept -> DerivedT * { return static_cast<DerivedT *>(this); }
};

}

#endif  //KB_NETWORKING_BASE_PACKET_SERIALIZER_H
