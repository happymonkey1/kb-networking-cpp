//
// Created by happymonkey1 on 8/16/25.
//

#ifndef KB_NETWORKING_CPP_KB_CORE_H
#define KB_NETWORKING_CPP_KB_CORE_H

#if defined(KB_NETWORKING_SHARED) && !defined(KB_API)
#  if defined(_WIN32) && !defined(__MINGW32__)
#    ifdef KB_API
#      define KB_API __declspec(dllexport)
#    else
#      define KB_API __declspec(dllimport)
#    endif
#  else
#    define KB_API __attribute ((visibility ("default")))
#  endif
#else
#  define KB_API
#endif

#ifndef KB_ABORT
#  define KB_ABORT(...) kb_abort(__FILE__, __LINE__, __VA_ARGS__)
#endif

#ifndef KB_ASSERT
#  define KB_ASSERT(x, ...) do { if (!(x)) KB_ABORT("Assertion failed ({}): {}", #x, __VA_ARGS__); } while (false)
#endif

#ifndef KB_UNUSED
#  define KB_UNUSED(x) (void)(x)
#endif

#ifdef __cplusplus
extern "C" {
#endif

KB_API void kb_abort(const char *p_file, int p_line, const char *p_fmt, ...);

#ifdef __cplusplus
#  ifndef KB_LOG_TRACE
#    define KB_LOG_TRACE(...) ::kb::core::Logger::get_core_logger()->trace(__VA_ARGS__)
#  endif

#  ifndef KB_LOG_DEBUG
#    define KB_LOG_DEBUG(...) ::kb::core::Logger::get_core_logger()->debug(__VA_ARGS__)
#  endif

#  ifndef KB_LOG_INFO
#    define KB_LOG_INFO(...) ::kb::core::Logger::get_core_logger()->info(__VA_ARGS__)
#  endif

#  ifndef KB_LOG_WARN
#    define KB_LOG_WARN(...) ::kb::core::Logger::get_core_logger()->warn(__VA_ARGS__)
#  endif

#  ifndef KB_LOG_ERROR
#    define KB_LOG_ERROR(...) ::kb::core::Logger::get_core_logger()->error(__VA_ARGS__)
#  endif

#  ifndef KB_LOG_CRITICAL
#    define KB_LOG_CRITICAL(...) ::kb::core::Logger::get_core_logger()->critical(__VA_ARGS__)
#  endif
#else

typedef enum kb_log_level {
  KB_LOG_LEVEL_NONE     = 0,
  KB_LOG_LEVEL_TRACE    = 1,
  KB_LOG_LEVEL_DEBUG    = 2,
  KB_LOG_LEVEL_INFO     = 3,
  KB_LOG_LEVEL_WARN     = 4,
  KB_LOG_LEVEL_ERROR    = 5,
  KB_LOG_LEVEL_CRITICAL = 6,
} kb_log_level;

KB_API void kb_log_internal(kb_log_level p_level, const char *p_fmt, ...);

#  ifndef KB_LOG_TRACE
#    define KB_LOG_TRACE(...) kb_log_internal(KB_LOG_LEVEL_TRACE, __VA_ARGS__)
#  endif

#  ifndef KB_LOG_DEBUG
#    define KB_LOG_DEBUG(...) kb_log_internal(KB_LOG_LEVEL_DEBUG, __VA_ARGS__)
#  endif

#  ifndef KB_LOG_INFO
#    define KB_LOG_INFO(...) kb_log_internal(KB_LOG_LEVEL_INFO, __VA_ARGS__)
#  endif

#  ifndef KB_LOG_WARN
#    define KB_LOG_WARN(...) kb_log_internal(KB_LOG_LEVEL_WARN, __VA_ARGS__)
#  endif

#  ifndef KB_LOG_ERROR
#    define KB_LOG_ERROR(...) kb_log_internal(KB_LOG_LEVEL_ERROR, __VA_ARGS__)
#  endif

#  ifndef KB_LOG_CRITICAL
#    define KB_LOG_CRITICAL(...) kb_log_internal(KB_LOG_LEVEL_CRITICAL, __VA_ARGS__)
#  endif

#endif

#ifdef __cplusplus
}
#endif

#endif  //KB_NETWORKING_CPP_KB_CORE_H
