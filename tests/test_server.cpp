#include "../src/kb/net/server.hpp"
#include "kb/kb_networking_cpp.hpp"

auto main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) -> int {
  kb_networking_init();
  KB_LOG_INFO("Initialized kb-networking library");

  auto server = kb::net::Server{};
  KB_LOG_INFO("Created server");
  server.start_async(11123);

  server.stop();

  KB_LOG_INFO("Destroying kb-networking library");
  kb_networking_shutdown();

  return 0;
}