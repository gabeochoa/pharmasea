
#pragma once

#include "engine/globals.h"

// YY / MM / DD (Monday of week)
constexpr std::string_view VERSION = "alpha_0.24.09.06";
constexpr size_t HASHED_VERSION = hashString(VERSION);

constexpr std::string_view SETTINGS_FILE_NAME = "settings.bin";

constexpr int MAP_H = 33;
// constexpr int MAP_W = 12;
// constexpr float WIN_RATIO = WIN_W * 1.f / WIN_H;

// TODO :INFRA: currently astar only supports tiles that are on the grid
// if you change this here, then go in there and add support as well
constexpr float TILESIZE = 1.0f;

struct Entity;

constexpr float GATHER_SPOT = -20.f;
constexpr int MAX_SEARCH_RANGE = 100;

static bool ENABLE_MODELS = true;
static bool ENABLE_SOUND = true;
static bool TESTS_ONLY = false;
static bool ENABLE_UI_TEST = false;

// TODO :BE: is there a way for us to move these to engine
// and then let the game pass them in or something _while_ staying const?
// and typed
namespace network {
constexpr int DEFAULT_PORT = 770;
constexpr int MAX_CLIENTS = 4;
constexpr int MAX_ANNOUNCEMENT_LENGTH = 200;
constexpr int SERVER_CLIENT_ID = -1;
constexpr int MAX_INPUTS = 100;

namespace mp_test {
static int run_init = 0;
static bool enabled = false;
static bool is_host = false;
}  // namespace mp_test

}  // namespace network
