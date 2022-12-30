
#pragma once

#include "engine/globals.h"

// YY / MM / DD (Monday of week)
constexpr std::string_view VERSION = "alpha_0.22.12.26";
constexpr size_t HASHED_VERSION = hashString(VERSION);

constexpr std::string_view GAME_FOLDER = "pharmasea";
constexpr std::string_view SETTINGS_FILE_NAME = "settings.bin";

constexpr int MAP_H = 33;
// constexpr int MAP_W = 12;
// constexpr float WIN_RATIO = WIN_W * 1.f / WIN_H;

// TODO currently astar only supports tiles that are on the grid
// if you change this here, then go in there and add support as well
constexpr float TILESIZE = 1.0f;

namespace network {
constexpr int DEFAULT_PORT = 770;
constexpr int MAX_CLIENTS = 4;
constexpr int MAX_ANNOUNCEMENT_LENGTH = 200;
constexpr int SERVER_CLIENT_ID = -1;
constexpr int MAX_INPUTS = 100;
}  // namespace network
