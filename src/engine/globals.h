

#pragma once

// TODO should be in vendor but we'd like it here
#include <mo_file/mo.h>

#include <string>

// BTW LOG_LEVEL is in there
#include "log.h"

// https://stackoverflow.com/a/48896410
template<typename Str>
[[nodiscard]] constexpr size_t hashString(const Str& toHash) {
    // NOTE: For this example, I'm requiring size_t to be 64-bit, but you could
    // easily change the offset and prime used to the appropriate ones
    // based on sizeof(size_t).
    static_assert(sizeof(size_t) == 8);
    // FNV-1a 64 bit algorithm
    size_t result = 0xcbf29ce484222325;  // FNV offset basis

    for (char c : toHash) {
        result ^= c;
        result *= 1099511628211;  // FNV prime
    }
    return result;
}

static int __WIN_H = 720;
static int __WIN_W = 1280;

[[nodiscard]] inline int WIN_W() { return __WIN_W; }
[[nodiscard]] inline float WIN_WF() { return static_cast<float>(__WIN_W); }

[[nodiscard]] inline int WIN_H() { return __WIN_H; }
[[nodiscard]] inline float WIN_HF() { return static_cast<float>(__WIN_H); }

namespace network {
// TODO add note for max name length in ui
constexpr int MAX_NAME_LENGTH = 25;
static long total_ping = 0;
static long there_ping = 0;
static long return_ping = 0;
}  // namespace network

static i18n::LocalizationText* localization;

// localization comes from engine/global.h
inline const char* text_lookup(const char* s) {
    if (!localization) {
        log_error("localization is nullptr");
        return s;
    }
    if (!localization->mo_data) {
        log_warn("localization not initialized");
        return s;
    }

    int target_index = get_target_index(localization, s);
    if (target_index == -1) {
        log_warn("localization missing {}", s);
        return s;
    }

    return get_translated_string(localization, target_index);
}
