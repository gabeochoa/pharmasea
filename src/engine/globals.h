

#pragma once

// TODO should be in vendor but we'd like it here
#include <mo_file/mo.h>

#include <string>

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

[[nodiscard]] constexpr size_t hashString(const char* toHash) {
    static_assert(sizeof(size_t) == 8,
                  "Only 64-bit size_t is supported for this example.");
    size_t result = 0xcbf29ce484222325;  // FNV offset basis
    const char* temp = toHash;
    while (*temp) {
        result ^= *temp;
        result *= 1099511628211;  // FNV prime
        ++temp;
    }
    return result;
}

static int __WIN_H = 720;
static int __WIN_W = 1280;

// Stored in preload.cpp
extern i18n::LocalizationText* localization;
extern int LOG_LEVEL;

[[nodiscard]] inline int WIN_W() { return __WIN_W; }
[[nodiscard]] inline float WIN_WF() { return static_cast<float>(__WIN_W); }

[[nodiscard]] inline int WIN_H() { return __WIN_H; }
[[nodiscard]] inline float WIN_HF() { return static_cast<float>(__WIN_H); }

namespace network {
// TODO add note for max name length in ui
constexpr int MAX_NAME_LENGTH = 25;
constexpr int MAX_SEED_LENGTH = 25;
static bool ENABLE_REMOTE_IP = false;
}  // namespace network
