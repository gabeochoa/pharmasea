
#pragma once

#include <iostream>

#include "log_level.h"
inline void log_me() { std::cout << std::endl; }
template<typename T, typename... Args>
inline void log_me(const T& arg, const Args&... args) {
    std::cout << arg << " ";
    log_me(args...);
}
#include "log_macros.h"
