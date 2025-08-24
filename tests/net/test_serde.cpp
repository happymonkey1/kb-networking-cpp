//
// Created by happy on 8/19/2025.
//

#include "../kb_testing.h"
#include "kb/kb_networking_cpp.hpp"
#include "kb/net/network_types.h"
#include "kb/net/udp/serde.hpp"
#include "kb/types.h"

#include <msgpack.hpp>

using namespace kb::net;

#include <string_view>

auto binary_serialize_packet_succeeds() -> void {
  KB_ASSERT_EQ(sizeof(details::internal_packet_header_t), 8, "Test expects internal packet header to be 8 bytes");

  constexpr std::string_view data = "kablunk";
  const auto packet = kb::net::serde::BinaryPacketSerializer::serialize(
    k_invalid_connection,
    data.data(),
    data.size()
  );

  KB_ASSERT_EQ(packet.m_conn, k_invalid_connection, "Unexpected connection value");
  KB_ASSERT_EQ(packet.m_header.m_serde_type, serialization_type_t::bin, "Unexpected serialization serializer_t");
  KB_ASSERT_EQ(
    packet.m_header.m_packet_type,
    static_cast<packet_type_t>(internal_packet_id_t::unknown),
    "Unexpected packet serializer_t"
  );

  constexpr auto k_expected_size =
    sizeof(details::internal_packet_header_t) +
    data.size();

  KB_ASSERT_EQ(
    k_expected_size,
    packet.m_buffer.size(),
    "Expected packet size %d, found %d instead",
    k_expected_size,
    packet.m_buffer.size()
  );

  const auto serialized_data = packet.m_buffer.data();
  std::size_t cursor = 0;
  KB_ASSERT_EQ(serialized_data[cursor++], k_magic_byte_0, "unexpected byte 0");
  KB_ASSERT_EQ(serialized_data[cursor++], k_magic_byte_1, "unexpected byte 1");
  KB_ASSERT_EQ(serialized_data[cursor++], k_magic_byte_2, "unexpected byte 2");
  KB_ASSERT_EQ(serialized_data[cursor++], static_cast<kb::u8>(packet.m_header.m_serde_type), "unexpected byte 3");
  KB_ASSERT_EQ(serialized_data[cursor++], static_cast<kb::u8>(packet.m_header.m_packet_type), "unexpected byte 4");
  KB_ASSERT_EQ(serialized_data[cursor++], static_cast<kb::u8>(packet.m_header.m_packet_type >> 8), "unexpected byte 5");
  KB_ASSERT_EQ(serialized_data[cursor++], static_cast<kb::u8>(packet.m_header.m_packet_type >> 16), "unexpected byte 6");
  KB_ASSERT_EQ(serialized_data[cursor++], static_cast<kb::u8>(packet.m_header.m_packet_type >> 24), "unexpected byte 7");
  for (; cursor < k_expected_size; ++cursor) {
    const char expected_char = data[cursor - sizeof(details::internal_packet_header_t)];
    const char found_char = reinterpret_cast<const char *>(serialized_data)[cursor];
    KB_LOG_INFO("Expected char: {:c}", expected_char);
    KB_LOG_INFO("Found char: {:c}", found_char);
    KB_ASSERT_EQ(
      found_char,
      expected_char,
      "Expected byte %c, found %c instead at byte %d",
      expected_char,
      found_char,
      cursor - sizeof(details::internal_packet_header_t)
    );
  }
}

auto binary_packet_serde_succeed() -> void {

  struct ping_data_t {
    kb::i32 packet_id = 1;
  };

  auto binary_serializer = kb::net::serde::BinaryPacketSerializer{};

  const auto conn = 12345;
  const auto packet_type = 1000;
  ping_data_t ping_data{
    .packet_id = 20
  };
  constexpr auto expected_packet_size = sizeof(kb::net::details::internal_packet_header_t) + sizeof(ping_data_t);
  auto binary_packet = binary_serializer.serialize_packet(conn, packet_type, std::move(ping_data));
  KB_ASSERT_EQ(
    binary_packet.m_buffer.size(),
    expected_packet_size,
    "Expected packet to be %d bytes, found %d instead",
    expected_packet_size,
    binary_packet.m_buffer.size()
  );

  const auto payload_size = binary_packet.m_buffer.size() - sizeof(kb::net::details::internal_packet_header_t);
  KB_ASSERT_EQ(payload_size, sizeof(ping_data_t), "Expected payload and ping_data_t size to match");
  auto deserialized_buffer  = binary_serializer.deserialize_packet(
    binary_packet.m_buffer.data() + sizeof(kb::net::details::internal_packet_header_t),
    binary_packet.m_buffer.size() - sizeof(kb::net::details::internal_packet_header_t)
  );

  KB_ASSERT_TRUE(deserialized_buffer.has_value(), "Expected deserialized buffer to have value");
  const auto buffer = *deserialized_buffer;
  KB_ASSERT_EQ(buffer.size(), sizeof(ping_data_t), "Expected deserialized buffer and ping_data_t size to match");

  ping_data_t deserialized_ping_data{};
  std::memcpy(&deserialized_ping_data, buffer.data(), buffer.size());

  KB_ASSERT_EQ(deserialized_ping_data.packet_id, ping_data.packet_id, "Expected deserialized ping data to match");
}

struct ping_data_t {
  kb::i32 packet_id = 1;

  MSGPACK_DEFINE(packet_id)
};

auto msgpack_packet_serde_succeeds() -> void {
  auto msgpack_serializer = kb::net::serde::MsgpackPacketSerializer{};

  const auto conn = 12345;
  const auto packet_type = 1000;
  ping_data_t expected_ping_data{
    .packet_id = 20
  };
  auto binary_packet = msgpack_serializer.serialize_packet(conn, packet_type, expected_ping_data);
  KB_ASSERT_EQ(
    binary_packet.m_buffer.size(),
    10,
    "Expected packet to be %d bytes, found %d instead",
    10,
    binary_packet.m_buffer.size()
  );

  auto maybe_ping_data = msgpack_serializer.deserialize_packet(
    binary_packet.m_buffer.data() + sizeof(kb::net::details::internal_packet_header_t),
    binary_packet.m_buffer.size() - sizeof(kb::net::details::internal_packet_header_t)
  );

  KB_ASSERT_TRUE(maybe_ping_data.has_value(), "Expected deserialized ping_data to have value");
  const auto & ping_data_object = *maybe_ping_data;

  const auto object = ping_data_object.get();
  ping_data_t ping_data;
  object.convert(ping_data);

  KB_ASSERT_EQ(ping_data.packet_id, expected_ping_data.packet_id, "Expected deserialized ping data to match");
}

auto main([[maybe_unused]] int argc, char ** argv) -> int {
  kb_networking_init();

  binary_serialize_packet_succeeds();
  KB_LOG_INFO("binary_serialize_packet_succeeds succeeded");
  binary_packet_serde_succeed();
  KB_LOG_INFO("binary_packet_serde_succeed succeeded");
  msgpack_packet_serde_succeeds();
  KB_LOG_INFO("msgpack_packet_serde_succeeds succeeded");

  kb_networking_shutdown();

  return 0;
}