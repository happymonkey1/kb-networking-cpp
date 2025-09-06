#include "../kb_testing.h"
#include "kb/kb_networking_cpp.hpp"
#include "kb/net/udp/client/udp_packet_client.hpp"
#include "kb/net/udp/server/udp_packet_server.hpp"
#include "kb/net/udp/udp.hpp"

enum class test_packet_type_t {
  Ping = 1000,
  Pong = 1001,
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
  constexpr auto k_expected_ping_id = 10;
  constexpr auto k_expected_pong_id = 20;

  kb_networking_init();
  KB_LOG_DEBUG("[test_client] Initialized kb-networking library");

  auto server = kb::net::UdpPacketServer<kb::net::serialization_type_t::msgpack>::create();
  KB_LOG_INFO("[test_client] Created server");
  const kb::u16 port = 12345;

  kb::u32 ping_count = 0;
  const auto bind_ping_res = server->bind_packet_handler(
    static_cast<kb::net::packet_type_t>(test_packet_type_t::Ping),
    [&ping_count, &server, k_expected_ping_id, k_expected_pong_id](
      kb_connection_t p_conn,
      const msgpack::object_handle & p_object
    ) -> void {
      KB_LOG_INFO("[test_client] Server handler received ping packet from conn: {}", static_cast<kb::u32>(p_conn));
      ++ping_count;

      ping_data_t ping_data{};
      try {
        const auto object = p_object.get();
        object.convert(ping_data);
      } catch (std::exception& err) {
        KB_LOG_ERROR("Test failed to deserialize expected ping data: {}", err.what());
        kb::core::Logger::get_core_logger()->flush();
        throw err;
      }

      KB_ASSERT_EQ(
        k_expected_ping_id,
        ping_data.packet_id,
        "Expected packet id %d, found %d",
        k_expected_ping_id,
        ping_data.packet_id
      );

      const auto pong_res = server->send(
        p_conn,
        static_cast<kb::net::packet_type_t>(test_packet_type_t::Pong),
        pong_data_t{ .packet_id = k_expected_pong_id },
        true
      );
      KB_ASSERT_TRUE(pong_res, "Failed to send pong packet to client");
    }
  );
  KB_ASSERT_TRUE(bind_ping_res, "Failed to bind ping packet handler to server");
  const auto start_async_res = server->start(port);
  KB_ASSERT_TRUE(start_async_res, "Failed to start async server");

  auto client = kb::net::udp::create_msgpack_client();
  KB_LOG_INFO("[test_client] Created client");
  kb::u32 pong_count = 0;
  const auto bind_pong_res = client->bind_packet_handler(
    static_cast<kb::net::packet_type_t>(test_packet_type_t::Pong),
    [&pong_count, &client, k_expected_pong_id](
      const msgpack::object_handle & p_object
    ) -> void {
      KB_LOG_INFO("[test_client] Received pong packet from server");
      ++pong_count;

      const auto object = p_object.get();

      pong_data_t pong_data{};
      object.convert(pong_data);

      KB_ASSERT_EQ(
        k_expected_pong_id,
        pong_data.packet_id,
        "Expected packet id %d, found: %d",
        k_expected_pong_id,
        pong_data.packet_id
      );
    }
  );
  KB_ASSERT_TRUE(bind_pong_res, "Failed to bind pong packet handler to client");

  const auto connect_res = client->connect("127.0.0.1", port);
  KB_ASSERT_TRUE(connect_res, "Failed to connect to server");

  constexpr kb::u32 max_conn_iters = 1000 / 10;
  kb::u32 conn_iter = 0;
  while (client->get_connection_status() != kb::net::UdpClient::connection_status_t::connected) {
    if (conn_iter++ >= max_conn_iters) {
      KB_ASSERT(false, "Failed to connect to host in 1 seconds");
      return -1;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  KB_ASSERT_EQ(
    kb::net::UdpClient::connection_status_t::connected,
    client->get_connection_status(),
    "Expected connected status, found: %d",
    static_cast<kb::u32>(client->get_connection_status())
  );

  const auto send_pack_res = client->send_packet(
    static_cast<kb::net::packet_type_t>(test_packet_type_t::Ping),
    ping_data_t{ .packet_id = k_expected_ping_id },
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

  client->stop();
  server->stop();

  KB_LOG_INFO("Destroying kb-networking library");
  kb_networking_shutdown();
}