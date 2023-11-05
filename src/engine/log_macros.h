#pragma once

#define log_trace(...)                    \
    if (LogLevel::LOG_TRACE >= LOG_LEVEL) \
    log_me(LogLevel::LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)

#define log_info(...)                    \
    if (LogLevel::LOG_INFO >= LOG_LEVEL) \
    log_me(LogLevel::LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...)                    \
    if (LogLevel::LOG_WARN >= LOG_LEVEL) \
    log_me(LogLevel::LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...)                                                \
    if (LogLevel::LOG_ERROR >= LOG_LEVEL)                             \
        log_me(LogLevel::LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__); \
    assert(false)

#define log_clean(level, ...) \
    if (level >= LOG_LEVEL) log_me(level, "", -1, __VA_ARGS__);

#define log_if(x, ...)                                                    \
    {                                                                     \
        if (x) log_me(LogLevel::LOG_IF, __FILE__, __LINE__, __VA_ARGS__); \
    }

#define log_ifx(x, level, ...)                                 \
    {                                                          \
        if (x) log_me(level, __FILE__, __LINE__, __VA_ARGS__); \
    }
