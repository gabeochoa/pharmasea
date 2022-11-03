
#pragma once

#include <string>

// YY / MM / DD (Monday of week)
const std::string VERSION = "alpha_0.22.10.31";

constexpr int WIN_H = 720;
constexpr int WIN_W = 1280;

constexpr int MAP_H = 33;
// constexpr int MAP_W = 12;
// constexpr float WIN_RATIO = WIN_W * 1.f / WIN_H;

// TODO currently astar only supports tiles that are on the grid
// if you change this here, then go in there and add support as well
constexpr float TILESIZE = 1.0f;

namespace network {

const int DEFAULT_PORT = 770;
const int MAX_CLIENTS = 4;
// TODO add note for max name length in ui
const int MAX_NAME_LENGTH = 25;
const int MAX_ANNOUNCEMENT_LENGTH = 200;
const int SERVER_CLIENT_ID = 0;

}  // namespace network
