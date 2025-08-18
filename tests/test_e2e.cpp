#include <steam/steamnetworkingtypes.h>

#include <string>

#include "kb/kb_networking_cpp.hpp"
#include "kb_testing.h"

enum class test_packet_type_t {
  Hello = 0,
  Goodbye = 1,
};

auto main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) noexcept -> int {
  kb_networking_init();
  KB_LOG_INFO("Initialized kb-networking library");

  auto server = kb::net::Server{};
  KB_LOG_INFO("Created server");
  server.start_async(11123);

  kb::u32 hello_packet_count = 0;
  kb::u32 goodbye_packet_count = 0;

  const auto bind_hello_res = server.bind_packet_handler(
    static_cast<kb::net::packet_type_t>(test_packet_type_t::Hello),
    [&hello_packet_count](HSteamNetConnection p_conn, const msgpack::object& p_object) -> void {
      KB_LOG_INFO("Server received Hello from {}", p_object.as<std::string>());
      ++hello_packet_count;
    }
  );
  KB_ASSERT_TRUE(bind_hello_res, "Failed to bind hello packet handler");

  const auto bind_goodbye_res = server.bind_packet_handler(
    static_cast<kb::net::packet_type_t>(test_packet_type_t::Goodbye),
    [&goodbye_packet_count](HSteamNetConnection p_conn, const msgpack::object& p_object) -> void {
      ++goodbye_packet_count;
    }
  );
  KB_ASSERT_TRUE(bind_goodbye_res, "Failed to bind goodbye packet handler");

  server.stop();

  KB_LOG_INFO("Destroying kb-networking library");
  kb_networking_shutdown();

  return 0;
}