//
// Created by happymonkey1 on 8/16/25.
//

#ifndef KB_NETWORKING_CPP_KB_LOG_H
#define KB_NETWORKING_CPP_KB_LOG_H

#include <spdlog/spdlog.h>

namespace kb::core {

class Logger {
public:
  static auto init() noexcept -> void;
  static auto shutdown() noexcept -> void;
  static auto get_core_logger() noexcept -> std::shared_ptr<spdlog::logger>;
};

}

#endif  //KB_NETWORKING_CPP_KB_LOG_H
