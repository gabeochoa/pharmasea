#pragma once

#include "../external_include.h"
//
#include "../menu.h"
#include "../util.h"

namespace network {

using Buffer = std::string;
using OutputAdapter = bitsery::OutputBufferAdapter<Buffer>;
using InputAdapter = bitsery::InputBufferAdapter<Buffer>;

const int DEFAULT_PORT = 770;
const int MAX_CLIENTS = 4;
// TODO add note for max name length in ui
const int MAX_NAME_LENGTH = 25;
const int MAX_ANNOUNCEMENT_LENGTH = 200;
const int SERVER_CLIENT_ID = 0;

struct Client_t {
    int client_id;
};

struct ClientPacket {
    int client_id;

    enum MsgType {
        Announcement,
        World,
        GameState,
        PlayerJoin,
        PlayerLocation,
    } msg_type;

    struct AnnouncementInfo {
        std::string message;
    };

    // World Info
    struct WorldInfo {};

    // Game Info
    struct GameStateInfo {
        Menu::State host_menu_state;
    };

    // Player Join
    // NOTE: anything added here is also added to player info
    struct PlayerJoinInfo {
        std::vector<int> all_clients;
        int client_id = -1;
        bool is_you = false;
    };

    // Player Location
    struct PlayerInfo {
        int facing_direction;
        float location[3];
        std::string name{};
    };

    typedef std::variant<ClientPacket::AnnouncementInfo,
                         ClientPacket::PlayerJoinInfo,
                         ClientPacket::GameStateInfo, ClientPacket::WorldInfo,
                         ClientPacket::PlayerInfo>
        Msg;

    Msg msg;
};

std::ostream& operator<<(std::ostream& os, const ClientPacket::Msg& msgtype) {
    os << std::visit(
        util::overloaded{
            [&](ClientPacket::AnnouncementInfo info) {
                return fmt::format("Announcement: {}", info.message);
            },
            [&](ClientPacket::PlayerJoinInfo info) {
                return fmt::format("PlayerJoinInfo( is_you: {}, id: {})",
                                   info.is_you, info.client_id);
            },
            [&](ClientPacket::GameStateInfo info) {
                return fmt::format("GameStateInfo( state: {} )",
                                   info.host_menu_state);
            },
            [&](ClientPacket::WorldInfo) { return std::string("worldinfo"); },
            [&](ClientPacket::PlayerInfo info) {
                return fmt::format(
                    "PlayerInfo( id{} name{} pos({}, {}, {}), facing {})",
                    info.name, info.location[0], info.location[1],
                    info.location[2], info.facing_direction);
            },
            [&](auto) { return std::string(" -- invalid operator<< --"); }},
        msgtype);
    return os;
}

std::ostream& operator<<(std::ostream& os,
                         const ClientPacket::MsgType& msgtype) {
    switch (msgtype) {
        case ClientPacket::Announcement:
            os << "Announcement";
            break;
        case ClientPacket::GameState:
            os << "GameState";
            break;
        case ClientPacket::World:
            os << "WorldInfo";
            break;
        case ClientPacket::PlayerLocation:
            os << "PlayerLocation";
            break;
        case ClientPacket::PlayerJoin:
            os << "PlayerJoinInfo";
            break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const ClientPacket& packet) {
    os << "Packet(" << packet.client_id << ": " << packet.msg_type << " "
       << packet.msg << ")" << std::endl;
    return os;
}

template<typename S>
void serialize(S& s, ClientPacket& packet) {
    s.value4b(packet.client_id);
    s.value4b(packet.msg_type);
    s.ext(packet.msg, bitsery::ext::StdVariant{
                          [](S& s, ClientPacket::AnnouncementInfo& info) {
                              s.text1b(info.message, MAX_ANNOUNCEMENT_LENGTH);
                          },
                          [](S& s, ClientPacket::PlayerJoinInfo& info) {
                              s.container4b(info.all_clients, MAX_CLIENTS);
                              s.value1b(info.is_you);
                              s.value4b(info.client_id);
                          },
                          [](S& s, ClientPacket::GameStateInfo& info) {
                              s.value4b(info.host_menu_state);
                          },
                          [](S&, ClientPacket::WorldInfo&) {},
                          [](S& s, ClientPacket::PlayerInfo& info) {
                              // end
                              s.text1b(info.name, MAX_NAME_LENGTH);
                              s.value4b(info.location[0]);
                              s.value4b(info.location[1]);
                              s.value4b(info.location[2]);
                              s.value4b(info.facing_direction);
                          },
                      });
}

}  // namespace network
