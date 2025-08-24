#include "kb/kb_networking_cpp.hpp"
#include "kb/net/udp/server/udp_server.hpp"
#include "../kb_testing.h"

auto main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) -> int {
  kb_networking_init();
  KB_LOG_INFO("Initialized kb-networking library");

  auto server = kb::net::UdpServer{};
  KB_LOG_INFO("Created server");
  server.start_async(11123);

  const std::string_view payload = "kablunk";
  const auto res = server.send(12345, payload.data(), payload.size());
  KB_ASSERT_EQ(res, false, "Expected send to fail since we have no client(s)");

  const auto broadcast_res = server.broadcast(payload.data(), payload.size());
  KB_ASSERT_EQ(broadcast_res, false, "Expected broadcast to fail since we have no client(s)");

  server.stop();

  KB_LOG_INFO("Destroying kb-networking library");
  kb_networking_shutdown();

  return 0;
}