#include "kb/kb_networking_cpp.hpp"

auto main() -> int {
  kb_networking_init();

  KB_LOG_INFO("Hello world!");

  kb_networking_shutdown();
}
