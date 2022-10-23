
#pragma once

#include <variant>
#include "../menu.h"

namespace network {

struct Info;

const int DEFAULT_PORT = 770;
const int MAX_CLIENTS = 3;
// TODO add some note somewhere about only
// supporting 50 character names
const int MAX_NAME_LENGTH = 25;

struct ClientPacket {
    int client_id;

    enum MsgType {
        Ping,
        PlayerJoin,
        GameState,
        World,
        PlayerLocation,
    } msg_type;

    // Ping
    struct PingInfo {};

    // PlayerJoin
    // NOTE:  Anything added here also gets added to playerinfo
    struct PlayerJoinInfo {
        bool is_you = false;
        int client_id = -1;
    };

    // Game
    struct GameStateInfo {
        Menu::State host_menu_state;
    };

    // World Info
    struct WorldInfo {};

    // Player Location
    struct PlayerInfo : public PlayerJoinInfo {
        std::string name{};
        float location[3];
        int facing_direction;
    };

    std::variant<PingInfo, PlayerJoinInfo, GameStateInfo, WorldInfo, PlayerInfo>
        msg;
};

}  // namespace network
