
#include "kb/core/logger.hpp"
#include "kb/kb_networking_cpp.hpp"

#include <steam/steamnetworkingsockets.h>

static bool s_initialized = false;

KB_API void kb_networking_init() {
  if (s_initialized) {
    return;
  }

  kb::core::Logger::init();
  if (SteamDatagramErrMsg err_msg; !GameNetworkingSockets_Init(nullptr, err_msg)) {
    KB_LOG_ERROR("Failed to initialize Steam GameNetworkingSockets: {}", err_msg);
    KB_ABORT("Failed to initialize Steam GameNetworkingSockets: %s", err_msg);
  }

  s_initialized = true;
}

KB_API void kb_networking_shutdown() {
  if (!s_initialized) {
    return;
  }

  GameNetworkingSockets_Kill();
  kb::core::Logger::shutdown();
}