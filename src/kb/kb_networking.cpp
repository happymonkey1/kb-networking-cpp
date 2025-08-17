
#include "kb/kb_networking.h"
#include "kb/core/logger.hpp"

KB_API void kb_networking_init() {
  kb::core::Logger::init();
}

KB_API void kb_networking_shutdown() {
  kb::core::Logger::shutdown();
}