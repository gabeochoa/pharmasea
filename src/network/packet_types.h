
#pragma once

#include <string>

#include "../engine/keymap.h"
#include "../engine/statemanager.h"
#include "../engine/toastmanager.h"
#include "../map.h"

namespace client_packet {

struct PingInfo {
    long long ping = 0;
    long long pong = 0;
};

struct AnnouncementInfo {
    std::string message;
    AnnouncementType type = AnnouncementType::Message;
};

// Map Info
struct MapInfo {
    struct Map map;
};

// Map Seed Info
struct MapSeedInfo {
    std::string seed{};
};

// Game Info
struct GameStateInfo {
    // TODO we likely dont need to send menu state anymore
    menu::State host_menu_state = menu::State::Game;
    game::State host_game_state = game::State::Lobby;
};

// Packet containing a recent keypress
struct PlayerControlInfo {
    UserInputs inputs;
};

// Player Join
// NOTE: anything added here is also added to player info
struct PlayerJoinInfo {
    std::vector<int> all_clients;
    int client_id = -1;
    size_t hashed_version = 0;
    bool is_you = false;
    std::string username{};
};

struct PlayerLeaveInfo {
    std::vector<int> all_clients;
    int client_id = -1;
};

// Player Location
struct PlayerInfo {
    float facing = 0.f;
    float location[3];
    std::string username{};
};

struct PlayerRareInfo {
    int client_id = -1;
    int model_index = 0;
    long long last_ping = -1;
};

struct PlaySoundInfo {
    float location[2];
    std::string sound;
};

}  // namespace client_packet
