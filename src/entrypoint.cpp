#include "kb/kb_networking_cpp.hpp"
#include "kb/core/logger.hpp"

auto main() -> int {
  kb::core::Logger::init();

  KB_LOG_INFO("Hello world!");

  kb::core::Logger::shutdown();
}
