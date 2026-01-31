

#pragma once

#include <memory>
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
        result ^= static_cast<size_t>(c);
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
        result ^= static_cast<size_t>(*temp);
        result *= 1099511628211;  // FNV prime
        ++temp;
    }
    return result;
}

// Stored in preload.cpp
extern int LOG_LEVEL;
extern int __WIN_H;
extern int __WIN_W;

// TODO: Remove these macros and query the window_manager::ProvidesCurrentResolution
// component directly everywhere. These are kept for now to minimize the migration scope.
[[nodiscard]] inline int WIN_W() { return __WIN_W; }
[[nodiscard]] inline float WIN_WF() { return static_cast<float>(__WIN_W); }

[[nodiscard]] inline int WIN_H() { return __WIN_H; }
[[nodiscard]] inline float WIN_HF() { return static_cast<float>(__WIN_H); }

namespace network {
// TODO add note for max name length in ui
constexpr int MAX_NAME_LENGTH = 25;
constexpr int MAX_SOUND_LENGTH = 25;
constexpr int MAX_SEED_LENGTH = 25;
// NOTE: these are runtime flags. They must be a single shared variable across
// translation units, so do NOT mark them `static` in a header.
inline bool ENABLE_REMOTE_IP = false;
// When true, the game runs in "local-only" mode:
// - no sockets/UDP required between host client and server
// - transport is in-process queues instead of GameNetworkingSockets
inline bool LOCAL_ONLY = false;
}  // namespace network
