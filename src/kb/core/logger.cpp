#include "../../../include/kb/core/logger.hpp"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace kb::core {

static std::shared_ptr<spdlog::logger> s_core_logger = nullptr;

auto Logger::init() noexcept -> void {
  std::vector<spdlog::sink_ptr> sinks;
  sinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
  sinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("kb-networking-cpp.log"));

  const auto default_log_level = spdlog::level::trace;

  sinks[0]->set_pattern("%^[%T] [Thread%5t] [%l] %n: %v%$");
  sinks[0]->set_level(default_log_level);
  sinks[1]->set_pattern("[%T] [Thread%5t] [%l] %n: %v");
  sinks[1]->set_level(spdlog::level::trace);

  s_core_logger = std::make_shared<spdlog::logger>("[kb-networking-cpp]", sinks.begin(), sinks.end());
  spdlog::register_logger(s_core_logger);
  s_core_logger->flush_on(default_log_level);
}

auto Logger::get_core_logger() noexcept -> std::shared_ptr<spdlog::logger> {
  return s_core_logger;
}
auto Logger::set_core_logger_level(const kb_log_level p_log_level) noexcept -> void {
  if (!s_core_logger) {
    return;
  }

  spdlog::level::level_enum spdlog_level;
  switch (p_log_level) {
    case KB_LOG_LEVEL_TRACE:    spdlog_level = spdlog::level::trace; break;
    case KB_LOG_LEVEL_DEBUG:    spdlog_level = spdlog::level::debug; break;
    case KB_LOG_LEVEL_INFO:     spdlog_level = spdlog::level::info; break;
    case KB_LOG_LEVEL_WARN:     spdlog_level = spdlog::level::warn; break;
    case KB_LOG_LEVEL_ERROR:    spdlog_level = spdlog::level::err; break;
    case KB_LOG_LEVEL_CRITICAL: spdlog_level = spdlog::level::critical; break;
    case KB_LOG_LEVEL_NONE:     [[fallthrough]];
    default:                    return;
  }

  auto& sinks = s_core_logger->sinks();
  for (auto& sink : sinks) {
    sink->set_level(spdlog_level);
  }
  s_core_logger->flush_on(spdlog_level);
}

auto Logger::shutdown() noexcept -> void {
  s_core_logger.reset();
}


}