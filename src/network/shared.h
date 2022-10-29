#pragma once

#include "../external_include.h"
//
#include <bitsery/adapter/buffer.h>
#include <bitsery/bitsery.h>
#include <bitsery/ext/std_tuple.h>
#include <bitsery/ext/std_variant.h>
#include <bitsery/traits/string.h>
#include <bitsery/traits/vector.h>

#include <cstring>
#include <variant>
//
#include "../util.h"

namespace network {

using Buffer = std::vector<unsigned char>;
using OutputAdapter = bitsery::OutputBufferAdapter<Buffer>;
using InputAdapter = bitsery::InputBufferAdapter<Buffer>;

const int DEFAULT_PORT = 7777;
const int MAX_CLIENTS = 32;

struct ClientPacket {
    int client_id;

    enum MsgType {
        Ping,
        World,
        PlayerLocation,
    } msg_type;

    // Ping
    struct PingInfo {};

    // World Info
    struct WorldInfo {};

    // Player Location
    struct PlayerInfo {
        float location[3];
        int facing_direction;
    };

    std::variant<PingInfo, WorldInfo, PlayerInfo> msg;
};

struct BaseInternal {
    std::map<int, ClientPacket::PlayerInfo> clients_to_process;
};

typedef std::variant<ClientPacket::PingInfo, ClientPacket::WorldInfo,
                     ClientPacket::PlayerInfo>
    Msg;

std::ostream& operator<<(std::ostream& os, const Msg& msgtype) {
    os << std::visit(
        util::overloaded{
            [&](ClientPacket::PingInfo) { return std::string("ping"); },
            [&](ClientPacket::WorldInfo) { return std::string("worldinfo"); },
            [&](ClientPacket::PlayerInfo info) {
                return fmt::format("PlayerInfo( pos({}, {}, {}), facing {})",
                                   info.location[0], info.location[1],
                                   info.location[2], info.facing_direction);
            },
            [&](auto) { return std::string(" -- invalid operator<< --"); }},
        msgtype);
    return os;
}

std::ostream& operator<<(std::ostream& os,
                         const ClientPacket::MsgType& msgtype) {
    switch (msgtype) {
        case ClientPacket::Ping:
            os << "Ping";
            break;
        case ClientPacket::World:
            os << "WorldInfo";
            break;
        case ClientPacket::PlayerLocation:
            os << "PlayerLocation";
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
                          [](S&, ClientPacket::PingInfo&) {},
                          [](S&, ClientPacket::WorldInfo&) {},
                          [](S& s, ClientPacket::PlayerInfo& info) {
                              s.value4b(info.location[0]);
                              s.value4b(info.location[1]);
                              s.value4b(info.location[2]);
                              s.value4b(info.facing_direction);
                          },
                      });
}

static void process_packet(std::shared_ptr<BaseInternal> internal,
                           ClientPacket packet) {
    // std::cout << packet << std::endl;
    if (packet.msg_type == ClientPacket::MsgType::PlayerLocation) {
        internal->clients_to_process[packet.client_id] =
            (std::get<ClientPacket::PlayerInfo>(packet.msg));
    }
}

}  // namespace network
