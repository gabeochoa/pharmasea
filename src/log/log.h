#pragma once

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <format>
#include <iostream>
#include <mutex>
#include <string>
#include <unordered_set>
#include <utility>

#include "../engine/globals.h"
#include "log_level.h"

namespace log_internal {
inline const char* level_label(LogLevel level) {
    switch (level) {
        case LogLevel::LOG_TRACE:
            return "TRACE";
        case LogLevel::LOG_DEBUG:
            return "DEBUG";
        case LogLevel::LOG_INFO:
            return "INFO";
        case LogLevel::LOG_WARN:
            return "WARN";
        case LogLevel::LOG_ERROR:
        default:
            return "ERROR";
    }
}
}  // namespace log_internal

template<typename... Args>
inline void log_with_level(LogLevel level, fmt::format_string<Args...> fmt_str,
                           Args&&... args) {
    if (static_cast<int>(level) < LOG_LEVEL) {
        return;
    }
    const auto message = fmt::format(fmt_str, std::forward<Args>(args)...);
    const char* label = log_internal::level_label(level);

    if (level == LogLevel::LOG_ERROR || level == LogLevel::LOG_WARN) {
        fmt::print(std::cerr, "[{}] {}\n", label, message);
    } else {
        fmt::print(std::cout, "[{}] {}\n", label, message);
    }
}

template<typename... Args>
inline void log_trace(fmt::format_string<Args...> fmt_str, Args&&... args) {
    log_with_level(LogLevel::LOG_TRACE, fmt_str, std::forward<Args>(args)...);
}

template<typename... Args>
inline void log_debug(fmt::format_string<Args...> fmt_str, Args&&... args) {
    log_with_level(LogLevel::LOG_DEBUG, fmt_str, std::forward<Args>(args)...);
}

template<typename... Args>
inline void log_info(fmt::format_string<Args...> fmt_str, Args&&... args) {
    log_with_level(LogLevel::LOG_INFO, fmt_str, std::forward<Args>(args)...);
}

template<typename... Args>
inline void log_warn(fmt::format_string<Args...> fmt_str, Args&&... args) {
    log_with_level(LogLevel::LOG_WARN, fmt_str, std::forward<Args>(args)...);
}

template<typename... Args>
inline void log_error(fmt::format_string<Args...> fmt_str, Args&&... args) {
    log_with_level(LogLevel::LOG_ERROR, fmt_str, std::forward<Args>(args)...);
    // This assert false is correct
    // it is used to stop the program exactly at the place we caught the error
    // so that its easier to debug
    // dont remove this assert
    assert(false);
}

// Logs without a level prefix (useful for already-formatted strings).
template<typename... Args>
inline void log_clean(fmt::format_string<Args...> fmt_str, Args&&... args) {
    fmt::print("{}\n", fmt::format(fmt_str, std::forward<Args>(args)...));
}

template<typename... Args>
inline void log_ifx(bool condition, LogLevel level,
                    fmt::format_string<Args...> fmt_str, Args&&... args) {
    if (condition) {
        log_with_level(level, fmt_str, std::forward<Args>(args)...);
    }
}

// Avoids spamming the same message repeatedly.
template<typename... Args>
inline void log_once_per(fmt::format_string<Args...> fmt_str, Args&&... args) {
    static std::mutex once_mutex;
    static std::unordered_set<std::string> seen;

    const auto message = fmt::format(fmt_str, std::forward<Args>(args)...);
    {
        std::lock_guard<std::mutex> lock(once_mutex);
        if (!seen.insert(message).second) {
            return;
        }
    }
    log_info("{}", message);
}

// Overload matching afterhours signature (Duration, log_level, fmt_str, args...)
// Used by vendor/afterhours/src/plugins/window_manager.h
template<typename Duration, typename... Args>
inline void log_once_per(Duration /*duration*/, int log_level,
                         std::format_string<Args...> fmt_str, Args&&... args) {
    // Map vendor log level to our LogLevel
    LogLevel level = LogLevel::LOG_INFO;
    if (log_level == 3) level = LogLevel::LOG_WARN;  // VENDOR_LOG_WARN
    else if (log_level == 4) level = LogLevel::LOG_ERROR;  // VENDOR_LOG_ERROR

    // TODO: Actually implement rate limiting with duration
    const auto message = std::format(fmt_str, std::forward<Args>(args)...);
    log_with_level(level, "{}", message);
}
