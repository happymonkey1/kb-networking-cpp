//
// Created by happymonkey1 on 8/16/25.
//

#include "kb/kb_core.h"
#include "kb/core/logger.hpp"
#include "types.h"

#include <cstdarg>

static char s_msg_buffer[2048] = {};

KB_API void kb_abort(const char *p_file, int p_line, const char* p_fmt, ...) {
  if (const auto logger = kb::core::Logger::get_core_logger(); logger) {
    char msg_buffer[2048] = {};
    std::va_list args;
    va_start(args, p_fmt);
    std::vsnprintf(msg_buffer, sizeof(msg_buffer), p_fmt, args);
    va_end(args);

    logger->critical("{}:{}: {}", p_file, p_line, msg_buffer);
  }

  std::abort();
}

KB_API const char * kb_format_str_internal(const char * p_fmt, ...) {
  std::va_list args;
  va_start(args, p_fmt);
  const auto end = std::vsnprintf(s_msg_buffer, sizeof(s_msg_buffer), p_fmt, args);
  va_end(args);
  s_msg_buffer[end] = 0;
  return s_msg_buffer;
}

KB_API void kb_log_internal_str(const kb_log_level p_level, const char *p_msg) {
  const auto logger = kb::core::Logger::get_core_logger();
  if (!logger) {
    return;
  }

  switch (p_level) {
    case KB_LOG_LEVEL_NONE:      return;
    case KB_LOG_LEVEL_TRACE:     logger->trace("{}", p_msg); break;
    case KB_LOG_LEVEL_DEBUG:     logger->debug("{}", p_msg); break;
    case KB_LOG_LEVEL_INFO:      logger->info("{}", p_msg); break;
    case KB_LOG_LEVEL_WARN:      logger->warn("{}", p_msg); break;
    case KB_LOG_LEVEL_ERROR:     logger->error("{}", p_msg); break;
    case KB_LOG_LEVEL_CRITICAL:  logger->critical("{}", p_msg); break;
  }
}

void kb_vlog_internal(const kb_log_level p_level, const char *p_fmt, va_list p_args) {
  if (!p_fmt) {
    return;
  }

  va_list args_copy;
  va_copy(args_copy, p_args);
  char msg_buffer[128];
  const kb::i32 len = vsnprintf(msg_buffer, sizeof(msg_buffer), p_fmt, args_copy);
  if (len < 128) {
    kb_log_internal_str(p_level, msg_buffer);
  } else {
    auto *msg_buffer_2 = (char *)calloc(len + 1, sizeof(char));
    vsnprintf(msg_buffer_2, len + 1, p_fmt, p_args);
    msg_buffer_2[len] = 0;
    kb_log_internal_str(p_level, msg_buffer_2);
    free(msg_buffer_2);
  }
}

KB_API void kb_log_internal(const kb_log_level p_level, const char *p_fmt, ...) {
  std::va_list args;
  va_start(args, p_fmt);
  kb_vlog_internal(p_level, p_fmt, args);
  va_end(args);
}
