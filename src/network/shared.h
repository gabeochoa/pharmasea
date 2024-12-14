#pragma once

#include "../external_include.h"
//

#include "../engine/keymap.h"
#include "../engine/time.h"
#include "../engine/toastmanager.h"
#include "../engine/util.h"
#include "../globals.h"
#include "../job.h"
#include "../level_info.h"
#include "../map.h"
#include "internal/channel.h"
#include "polymorphic_components.h"
#include "steam/steamnetworkingtypes.h"

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

        template<class Archive>
        void serialize(Archive& archive) {
            archive(ping, pong);
        }
    };

    struct AnnouncementInfo {
        std::string message;
        AnnouncementType type = AnnouncementType::Message;

        template<class Archive>
        void serialize(Archive& archive) {
            archive(message, type);
        }
    };

    // Map Info
    struct MapInfo {
        struct Map map;

        template<class Archive>
        void serialize(Archive& archive) {
            archive(map);
        }
    };

    // Map Seed Info
    struct MapSeedInfo {
        std::string seed{};

        template<class Archive>
        void serialize(Archive& archive) {
            archive(seed);
        }
    };

    // Game Info
    struct GameStateInfo {
        // TODO we likely dont need to send menu state anymore
        menu::State host_menu_state = menu::State::Game;
        game::State host_game_state = game::State::Lobby;

        template<class Archive>
        void serialize(Archive& archive) {
            archive(host_game_state, host_menu_state);
        }
    };

    // Packet containing a recent keypress
    struct PlayerControlInfo {
        UserInputs inputs;

        template<class Archive>
        void serialize(Archive& archive) {
            archive(inputs);
        }
    };

    // Player Join
    // NOTE: anything added here is also added to player info
    struct PlayerJoinInfo {
        std::vector<int> all_clients;
        int client_id = -1;
        size_t hashed_version = 0;
        bool is_you = false;
        std::string username{};

        template<class Archive>
        void serialize(Archive& archive) {
            archive(all_clients, is_you, hashed_version, client_id, username);
        }
    };

    struct PlayerLeaveInfo {
        std::vector<int> all_clients;
        int client_id = -1;

        template<class Archive>
        void serialize(Archive& archive) {
            archive(all_clients, client_id);
        }
    };

    // Player Location
    struct PlayerInfo {
        float facing = 0.f;
        float location[3];
        std::string username{};

        template<class Archive>
        void serialize(Archive& archive) {
            archive(username, location, facing);
        }
    };

    struct PlayerRareInfo {
        int client_id = -1;
        int model_index = 0;
        long long last_ping = -1;

        template<class Archive>
        void serialize(Archive& archive) {
            archive(client_id, model_index, last_ping);
        }
    };

    struct PlaySoundInfo {
        float location[2];
        std::string sound;
        template<class Archive>
        void serialize(Archive& archive) {
            archive(location, sound);
        }
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

    template<class Archive>
    void serialize(Archive& archive) {
        archive(channel, client_id, msg_type, msg);
    }
};

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
                                   info.location[1], info.sound);
            },
            [&](auto) { return std::string(" -- invalid operator<< --"); }}
        // namespace network
        ,
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

static Buffer serialize_to_entity(Entity* entity) {
    std::stringstream ss;
    {
        cereal::JSONOutputArchive archive(ss);
        archive(*entity);
    }
    return ss.str();
}

static void deserialize_to_entity(Entity* entity, const std::string& msg) {
    std::stringstream ss(msg);
    {
        cereal::JSONInputArchive archive(ss);
        archive(*entity);
    }
}

// ClientPacket is in shared.h which is specific to the game,
// TODO how can we support both abstract while also configuration
static ClientPacket deserialize_to_packet(const std::string& msg) {
    ClientPacket packet;
    std::stringstream ss(msg);
    {
        cereal::JSONInputArchive archive(ss);
        archive(packet);
    }
    return packet;
}

static Buffer serialize_to_buffer(ClientPacket packet) {
    std::stringstream ss;
    {
        cereal::JSONOutputArchive archive(ss);
        archive(packet);
    }
    return ss.str();
}

}  // namespace network
