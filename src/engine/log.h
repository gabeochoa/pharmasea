

#pragma once

#include <cassert>

enum class LogLevel {
    ALL = 0,
    TRACE = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
};

// TODO log to file

// TODO right now having some issues with MSVC and not getting any decent error
// message
#ifdef __APPLE__
#include <functional>
#include <iostream>
#include <string>

#include "../external_include.h"
#include "globals.h"

inline const std::string_view level_to_string(LogLevel level) {
    return magic_enum::enum_name(level);
}

inline void vlog(LogLevel level, const char* file, int line,
                 fmt::string_view format, fmt::format_args args) {
    if ((int) level < LOG_LEVEL) return;
    fmt::print("{}: {}: {}: ", file, line, level_to_string(level));
    fmt::vprint(format, args);
    fmt::print("\n");
}

template<typename... Args>
inline void log_me(LogLevel level, const char* file, int line,
                   const char* format, Args&&... args) {
    vlog(level, file, line, format,
         fmt::make_args_checked<Args...>(format, args...));
}

template<typename... Args>
inline void log_me(LogLevel level, const char* file, int line,
                   const wchar_t* format, Args&&... args) {
    vlog(level, file, line, format,
         fmt::make_args_checked<Args...>(format, args...));
}

template<>
inline void log_me(LogLevel level, const char* file, int line,
                   const char* format, const char*&& args) {
    vlog(level, file, line, format,
         fmt::make_args_checked<const char*>(format, args));
}

#else
// TODO implement for windows
static void log_me(...) {}
#endif

#define log_trace(...) log_me(LogLevel::TRACE, __FILE__, __LINE__, __VA_ARGS__)

#define log_info(...) log_me(LogLevel::INFO, __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...) log_me(LogLevel::WARN, __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...)                                        \
    log_me(LogLevel::ERROR, __FILE__, __LINE__, __VA_ARGS__); \
    assert(false)
