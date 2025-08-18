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

  KB_LOG_DEBUG("fucjk you");

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
  kb::u32 pong_count = 0;
  const auto bind_pong_res = client.bind_handler(
    static_cast<kb::net::packet_type_t>(test_packet_type_t::Pong),
    [&pong_count, &client](
      msgpack::object p_object
    ) -> void {
      KB_LOG_INFO("Received pong packet from server");
      ++pong_count;

      pong_data_t pong_data{};
      p_object.convert(pong_data);

      KB_ASSERT_EQ(1, pong_data.packet_id, "Expected packet id 1, found: %d", pong_data.packet_id);
    }
  );
  KB_ASSERT_TRUE(bind_pong_res, "Failed to bind pong packet handler to client");

  const auto connect_res = client.connect("127.0.0.1", port);
  KB_ASSERT_TRUE(connect_res, "Failed to connect to server");

  constexpr kb::u32 max_conn_iters = 1000 / 10;
  kb::u32 conn_iter = 0;
  while (client.get_connection_status() != kb::net::Client::connection_status_t::connected) {
    if (conn_iter++ >= max_conn_iters) {
      KB_ASSERT(false, "Failed to connect to host in 1 seconds");
      return -1;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  KB_ASSERT_EQ(
    kb::net::Client::connection_status_t::connected,
    client.get_connection_status(),
    "Expected connected status, found: %d",
    client.get_connection_status()
  );

  const auto send_pack_res = client.send_packet(
    static_cast<kb::net::packet_type_t>(test_packet_type_t::Ping),
    ping_data_t{ .packet_id = 0 },
    true
  );
  KB_ASSERT_TRUE(send_pack_res, "Failed to send ping packet to server");

  constexpr kb::u32 max_iters = 1000 / 10;
  kb::u32 iter = 0;
  while (pong_count == 0) {
    if (iter++ >= max_iters) {
      KB_ASSERT(false, "Failed to wait for pong response");
    }
    // Waiting for pong
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  KB_ASSERT_EQ(1, ping_count, "Expected 1 ping, found: %d", ping_count);
  KB_ASSERT_EQ(1, pong_count, "Expected 1 pong, found: %d", pong_count);

  client.stop();
  server.stop();

  KB_LOG_INFO("Destroying kb-networking library");
  kb_networking_shutdown();
}