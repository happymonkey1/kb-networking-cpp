

#include "kb/core/logger.h"
auto main() -> int {
  kb::Logger::init();

  kb::Logger::get_core_logger()->info("Hello world!");

  kb::Logger::shutdown();
}
