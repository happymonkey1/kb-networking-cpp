

//
// Created by happymonkey1 on 8/18/25.
//

#ifndef KB_NETWORKING_SERDE_HPP
#define KB_NETWORKING_SERDE_HPP

#include "kb/net/serde/base_packet_serializer.h"
#include "kb/net/serde/binary_packet_serializer.h"
#include "kb/net/serde/msgpack_packet_serializer.h"

#include <msgpack.hpp>

namespace kb::net::serde {

template <serialization_type_t>
struct serializer_traits;

template <>
struct serializer_traits<serialization_type_t::bin> {
  using serializer_t = BinaryPacketSerializer;
  using payload_t = BinaryPacketSerializer::payload_t;
};

template <>
struct serializer_traits<serialization_type_t::msgpack> {
  using serializer_t = MsgpackPacketSerializer;
  using payload_t = MsgpackPacketSerializer::payload_t;
};

} // end namespace kb::net::serde

#endif  //KB_NETWORKING_SERDE_HPP

