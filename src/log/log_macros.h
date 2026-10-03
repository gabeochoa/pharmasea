#pragma once

#include "log.h"

#define LOG_TRACE(...) log_trace(__VA_ARGS__)
#define LOG_DEBUG(...) log_debug(__VA_ARGS__)
#define LOG_INFO(...) log_info(__VA_ARGS__)
#define LOG_WARN(...) log_warn(__VA_ARGS__)
#define LOG_ERROR(...) log_error(__VA_ARGS__)

#define VALIDATE(condition, ...)    \
    do {                            \
        if (!(condition)) {         \
            log_error(__VA_ARGS__); \
        }                           \
    } while (0)
