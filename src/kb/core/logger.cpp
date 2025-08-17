#include "../../../include/kb/core/logger.hpp"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace kb::core {

static std::shared_ptr<spdlog::logger> s_core_logger = nullptr;

auto Logger::init() noexcept -> void {
  std::vector<spdlog::sink_ptr> sinks;
  sinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
  sinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("kb-networking-cpp.log"));

  sinks[0]->set_pattern("%^[%T] %n: %v%$");
  sinks[0]->set_level(spdlog::level::info);
  sinks[1]->set_pattern("[%T] [Thread%5t] [%l] %n: %v");
  sinks[1]->set_level(spdlog::level::trace);

  s_core_logger = std::make_shared<spdlog::logger>("[kb-networking-cpp]", sinks.begin(), sinks.end());
  spdlog::register_logger(s_core_logger);
  s_core_logger->flush_on(spdlog::level::info);
}

auto Logger::get_core_logger() noexcept -> std::shared_ptr<spdlog::logger> {
  return s_core_logger;
}

auto Logger::shutdown() noexcept -> void {
  s_core_logger.reset();
}


}