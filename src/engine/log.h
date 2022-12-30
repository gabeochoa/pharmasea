

#pragma once

#include <cassert>
#include <functional>
#include <iostream>
#include <string>

#include "../external_include.h"
#include "globals.h"

enum LogLevel : int {
    ALL = 0,
    TRACE = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
};

inline const char* level_to_string(int level) {
    switch (level) {
        default:
        case LogLevel::ALL:
            return "";
        case LogLevel::TRACE:
            return "Trace";
        case LogLevel::INFO:
            return "Info";
        case LogLevel::WARN:
            return "Warn";
        case LogLevel::ERROR:
            return "Error";
    }
}

// TODO log to file

inline void vlog(int level, const char* file, int line, fmt::string_view format,
                 fmt::format_args args) {
    if (level < LOG_LEVEL) return;
    fmt::print("{}: {}: {}: ", file, line, level_to_string(level));
    fmt::vprint(format, args);
    fmt::print("\n");
}

inline void vlog(int level, const char* file, int line,
                 fmt::wstring_view format, fmt::wformat_args args) {
    if (level < LOG_LEVEL) return;
    fmt::print("{}: {}: {}: ", file, line, level_to_string(level));
    fmt::vprint(format, args);
    fmt::print("\n");
}

template<typename... Args>
inline void log_me(int level, const char* file, int line, const char* format,
                   Args&&... args) {
    vlog(level, file, line, format,
         fmt::make_args_checked<Args...>(format, args...));
}

template<typename... Args>
inline void log_me(int level, const char* file, int line, const wchar_t* format,
                   Args&&... args) {
    vlog(level, file, line, format,
         fmt::make_args_checked<Args...>(format, args...));
}

template<>
inline void log_me(int level, const char* file, int line, const char* format,
                   const char*&& args) {
    vlog(level, file, line, format,
         fmt::make_args_checked<const char*>(format, args));
}

template<>
inline void log_me(int level, const char* file, int line, const wchar_t* format,
                   const wchar_t*&& args) {
    vlog(level, file, line, format,
         fmt::make_args_checked<const wchar_t*>(format, args));
}

#define log_trace(...) log_me(LogLevel::TRACE, __FILE__, __LINE__, __VA_ARGS__)

#define log_info(...) log_me(LogLevel::INFO, __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...) log_me(LogLevel::WARN, __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...)                                        \
    log_me(LogLevel::ERROR, __FILE__, __LINE__, __VA_ARGS__); \
    assert(false)
