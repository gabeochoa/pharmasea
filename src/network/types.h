#pragma once

#include <iostream>
#include <string>
#include <variant>
#include <vector>

#include "../engine/keymap.h"  // for UserInputs
#include "../engine/statemanager.h"
#include "../engine/toastmanager.h"  // for AnnouncementType
#include "../engine/util.h"
#include "../external_include.h"  // for fmt, magic_enum
#include "../strings.h"
#include "internal/channel.h"
#include "../world_snapshot_v2.h"
#include "../map.h"

namespace network {

using Buffer = std::string;

struct ClientPacket {
    Channel channel = Channel::RELIABLE;
    int client_id;

    enum MsgType {
        Announcement,
        Map,
        MapSeed,
        GameState,
        PlayerControl,
        PlayerJoin,
        PlayerLeave,
        PlayerLocation,
        PlayerRare,
        Ping,
        PlaySound,
    } msg_type;

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
        enum class Kind : std::uint8_t {
            Begin = 0,
            Chunk = 1,
            End = 2,
        };

        Kind kind = Kind::Begin;
        std::uint32_t snapshot_id = 0;

        // Total size of the serialized snapshot payload (bytes).
        std::uint32_t total_size = 0;

        // For Begin: current map UI state.
        bool showMinimap = false;

        // For Chunk: byte offset into payload + data bytes.
        std::uint32_t offset = 0;
        std::string data{};
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
        strings::sounds::SoundId sound;
    };

    using Msg =
        std::variant<ClientPacket::AnnouncementInfo,
                     ClientPacket::PlayerControlInfo,
                     ClientPacket::PlayerJoinInfo, ClientPacket::GameStateInfo,
                     ClientPacket::MapInfo, ClientPacket::MapSeedInfo,
                     ClientPacket::PlayerInfo, ClientPacket::PlayerLeaveInfo,
                     ClientPacket::PlayerRareInfo, ClientPacket::PingInfo,
                     ClientPacket::PlaySoundInfo>;

    Msg msg;
};

// Stream operators (lightweight, only need fmt and magic_enum)
inline std::ostream& operator<<(std::ostream& os,
                                const ClientPacket::Msg& msgtype) {
    os << std::visit(
        util::overloaded{
            [&](const ClientPacket::AnnouncementInfo& info) {
                return fmt::format("Announcement: {}", info.message);
            },
            [&](const ClientPacket::PlayerJoinInfo& info) {
                return fmt::format(
                    "PlayerJoinInfo( is_you: {}, id: {}, username {})",
                    info.is_you, info.client_id, info.username);
            },
            [&](const ClientPacket::PlayerControlInfo& info) {
                return fmt::format("PlayerControlInfo( num_inputs: {})",
                                   info.inputs.size());
            },
            [&](const ClientPacket::GameStateInfo& info) {
                return fmt::format("GameStateInfo( menu: {} game: {})",
                                   info.host_menu_state, info.host_game_state);
            },
            [&](const ClientPacket::MapInfo&) {
                return std::string("map info");
            },
            [&](const ClientPacket::MapSeedInfo& s) {
                return fmt::format("Map Seed: {}", s.seed);
            },
            [&](const ClientPacket::PlayerInfo& info) {
                return fmt::format("PlayerInfo( pos({}, {}, {}), facing {})",
                                   info.location[0], info.location[1],
                                   info.location[2], info.facing);
            },
            [&](const ClientPacket::PlayerLeaveInfo& info) {
                return fmt::format("PlayerLeave({})", info.client_id);
            },
            [&](const ClientPacket::PlayerRareInfo& info) {
                return fmt::format("PlayerRare({})", info.client_id);
            },
            [&](const ClientPacket::PingInfo& info) {
                return fmt::format("Ping({}, {})", info.ping, info.pong);
            },
            [&](const ClientPacket::PlaySoundInfo& info) {
                return fmt::format("PlaySound({} {}, {})", info.location[0],
                                   info.location[1], (int) info.sound);
            },
            [&](auto) { return std::string(" -- invalid operator<< --"); }},
        msgtype);
    return os;
}

inline std::ostream& operator<<(std::ostream& os,
                                const ClientPacket::MsgType& msgtype) {
    os << magic_enum::enum_name(msgtype);
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const ClientPacket& packet) {
    os << "Packet(" << packet.client_id << ": " << packet.msg_type << " "
       << packet.msg << ")" << std::endl;
    return os;
}

}  // namespace network
