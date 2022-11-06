#pragma once

#include "../external_include.h"
//
#include "../globals.h"
#include "../keymap.h"
#include "../menu.h"
#include "../util.h"
#include "steam/steamnetworkingtypes.h"

namespace bitsery {
template<typename S>
void serialize(S& s, GamepadAxisWithDir& input) {
    s.value4b(input.axis);
    s.value4b(input.dir);
}

}  // namespace bitsery

namespace network {

using Buffer = std::string;
using OutputAdapter = bitsery::OutputBufferAdapter<Buffer>;
using InputAdapter = bitsery::InputBufferAdapter<Buffer>;

enum Channel {
    RELIABLE = k_nSteamNetworkingSend_Reliable,
    UNRELIABLE = k_nSteamNetworkingSend_Unreliable,
    UNRELIABLE_NO_DELAY = k_nSteamNetworkingSend_UnreliableNoDelay,
};

struct Client_t {
    int client_id;
};

struct ClientPacket {
    Channel channel = Channel::RELIABLE;
    int client_id;

    enum MsgType {
        Announcement,
        World,
        GameState,
        PlayerControl,
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

    // Packet containing a recent keypress
    struct PlayerControlInfo {
        UserInputs inputs;
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

    typedef std::variant<
        ClientPacket::AnnouncementInfo, ClientPacket::PlayerControlInfo,
        ClientPacket::PlayerJoinInfo, ClientPacket::GameStateInfo,
        ClientPacket::WorldInfo, ClientPacket::PlayerInfo>
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
            [&](ClientPacket::PlayerControlInfo info) {
                return fmt::format("PlayerControlInfo( num_inputs: {})",
                                   info.inputs.size());
            },
            [&](ClientPacket::GameStateInfo info) {
                return fmt::format("GameStateInfo( state: {} )",
                                   info.host_menu_state);
            },
            [&](ClientPacket::WorldInfo) { return std::string("worldinfo"); },
            [&](ClientPacket::PlayerInfo info) {
                return fmt::format(
                    "PlayerInfo( name{} pos({}, {}, {}), facing {})", info.name,
                    info.location[0], info.location[1], info.location[2],
                    info.facing_direction);
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
            os << "PlayerJoin";
            break;
        case ClientPacket::PlayerControl:
            os << "PlayerControl";
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
    s.value4b(packet.channel);
    s.value4b(packet.client_id);
    s.value4b(packet.msg_type);
    s.ext(packet.msg,
          bitsery::ext::StdVariant{
              [](S& s, ClientPacket::AnnouncementInfo& info) {
                  s.text1b(info.message, MAX_ANNOUNCEMENT_LENGTH);
              },
              [](S& s, ClientPacket::PlayerJoinInfo& info) {
                  s.container4b(info.all_clients, MAX_CLIENTS);
                  s.value1b(info.is_you);
                  s.value4b(info.client_id);
              },
              [](S& s, ClientPacket::PlayerControlInfo& info) {
                  s.container(
                      info.inputs, MAX_INPUTS, [](S& sv, UserInput& input) {
                          sv.ext(
                              input,
                              bitsery::ext::StdTuple{
                                  [](auto& s, Menu::State& o) { s.value4b(o); },
                                  [](auto& s, std::string& o) {
                                      s.text1b(o, 100);
                                  },
                                  [](auto& s, float& o) { s.value4b(o); }});
                      });
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
