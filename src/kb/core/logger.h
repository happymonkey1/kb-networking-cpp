#ifndef KB_CORE_LOGGER_H
#define KB_CORE_LOGGER_H

#include <spdlog/spdlog.h>

namespace kb {

class Logger {
public:
  static auto init() noexcept -> void;
  static auto shutdown() noexcept -> void;
  static auto get_core_logger() noexcept -> std::shared_ptr<spdlog::logger>;
};

}

#endif
