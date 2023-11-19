
#pragma once

#include "globals.h"  // where LOG_LEVEL is located

enum LogLevel {
    LOG_ALOG_ = 0,
    LOG_TRACE = 1,
    LOG_INFO = 2,
    LOG_WARN = 3,
    LOG_ERROR = 4,
    LOG_IF = 5,
    LOG_NOTHING = 6,
};
