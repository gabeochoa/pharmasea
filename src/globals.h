
#pragma once

#include <string>

// https://stackoverflow.com/a/48896410
template<typename Str>
constexpr size_t hashString(const Str& toHash) {
    // For this example, I'm requiring size_t to be 64-bit, but you could
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

// YY / MM / DD (Monday of week)
constexpr std::string_view VERSION = "alpha_0.22.11.07";
constexpr size_t HASHED_VERSION = hashString(VERSION);

constexpr int WIN_H = 720;
constexpr int WIN_W = 1280;

constexpr int MAP_H = 33;
// constexpr int MAP_W = 12;
// constexpr float WIN_RATIO = WIN_W * 1.f / WIN_H;

// TODO currently astar only supports tiles that are on the grid
// if you change this here, then go in there and add support as well
constexpr float TILESIZE = 1.0f;

namespace network {

constexpr int DEFAULT_PORT = 770;
constexpr int MAX_CLIENTS = 4;
// TODO add note for max name length in ui
constexpr int MAX_NAME_LENGTH = 25;
constexpr int MAX_ANNOUNCEMENT_LENGTH = 200;
constexpr int SERVER_CLIENT_ID = -1;
constexpr int MAX_INPUTS = 100;
}  // namespace network
