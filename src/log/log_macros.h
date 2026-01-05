#pragma once

#include "log.h"

#define LOG_TRACE(...) log_trace(__VA_ARGS__)
#define LOG_DEBUG(...) log_debug(__VA_ARGS__)
#define LOG_INFO(...) log_info(__VA_ARGS__)
#define LOG_WARN(...) log_warn(__VA_ARGS__)
#define LOG_ERROR(...) log_error(__VA_ARGS__)

// Prefer using `INVARIANT`/`VALIDATE` from `engine/assert.h` for contracts.
// This header is used by `engine/log_fakelog.h` and may be included in contexts
// where assert helpers are intentionally absent.
#define LOG_VALIDATE(condition, ...) \
    do {                             \
        if (!(condition)) {          \
            log_error(__VA_ARGS__);  \
        }                            \
    } while (0)

// Avoid macro redefinition if `engine/assert.h` is also included.
#ifndef VALIDATE
#define VALIDATE(condition, ...) LOG_VALIDATE(condition, __VA_ARGS__)
#endif
