#include "kb/kb_networking_cpp.hpp"
#include "kb_testing.h"

enum class test_packet_type_t {
  Ping = 0,
  Pong = 1,
};

struct ping_data_t {
  kb::i32 packet_id;

  MSGPACK_DEFINE(packet_id)
};

struct pong_data_t {
  kb::i32 packet_id;

  MSGPACK_DEFINE(packet_id)
};

auto main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) -> int {
  kb_networking_init();
  kb::core::Logger::set_core_logger_level(KB_LOG_LEVEL_TRACE);
  KB_LOG_INFO("Initialized kb-networking library");

  auto server = kb::net::Server{};
  KB_LOG_INFO("Created server");
  const kb::u16 port = 12345;

  kb::u32 ping_count = 0;
  const auto bind_ping_res = server.bind_packet_handler(
    static_cast<kb::net::packet_type_t>(test_packet_type_t::Ping),
    [&ping_count, &server](
      HSteamNetConnection p_conn,
      const msgpack::object& p_object
    ) -> void {
      KB_LOG_INFO("Received ping packet from conn: {}", static_cast<kb::u32>(p_conn));
      ++ping_count;

      ping_data_t ping_data{};
      p_object.convert(ping_data);

      KB_LOG_INFO("Wait 500 ms before sending pong");
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      KB_ASSERT_EQ(0, ping_data.packet_id, "Expected packet id 0, found %d", ping_data.packet_id);

      const auto pong_res = server.send(
        p_conn,
        static_cast<kb::net::packet_type_t>(test_packet_type_t::Pong),
        pong_data_t{ .packet_id = 1 },
        true
      );
      KB_ASSERT_TRUE(pong_res, "Failed to send pong packet to client");
    }
  );
  KB_ASSERT_TRUE(bind_ping_res, "Failed to bind ping packet handler to server");
  const auto start_async_res = server.start_async(port);
  KB_ASSERT_TRUE(start_async_res, "Failed to start async server");

  auto client = kb::net::Client{};
  KB_LOG_INFO("Created client");
  const auto connect_res = coro::sync_wait(client.async_connect("127.0.0.1", port));
  KB_ASSERT_TRUE(connect_res, "Failed to connect to server");
  KB_LOG_INFO("Client connection status: {}", static_cast<kb::u32>(client.get_connection_status()));

  KB_ASSERT_EQ(
    kb::net::Client::connection_status_t::connected,
    client.get_connection_status(),
    "Expected connected status, found: %d",
    client.get_connection_status()
  );

  // Send ping
  const auto send_pack_res = client.send_packet(
    static_cast<kb::net::packet_type_t>(test_packet_type_t::Ping),
    ping_data_t{ .packet_id = 0 },
    true
  );
  KB_ASSERT_TRUE(send_pack_res, "Failed to send ping packet to server");
  KB_LOG_INFO("Ping packet sent");

  // Waiting for pong response
  KB_LOG_INFO("Waiting for pong response");
  const auto pong_res = coro::sync_wait(client.async_wait_for_packet(
    static_cast<kb::net::packet_type_t>(test_packet_type_t::Pong)
  ));
  KB_ASSERT_TRUE(pong_res.has_value(), "Pong response needs a value");
  KB_LOG_INFO("Pong packet received");

  pong_data_t pong_data{};
  try {
    pong_res->convert(pong_data);
  } catch (const std::exception& err) {
    KB_ASSERT(false, "Failed to convert pong response: %s", err.what());
    return -1;
  }

  KB_ASSERT_EQ(1, pong_data.packet_id, "Expected pong to have packet id 1");
  KB_ASSERT_EQ(1, ping_count, "Expected 1 ping, found: %d", ping_count);

  KB_LOG_INFO("Destroying client");
  client.stop();
  KB_LOG_INFO("Destroying server");
  server.stop();

  KB_LOG_INFO("Destroying kb-networking library");
  kb_networking_shutdown();

  return 0;
}