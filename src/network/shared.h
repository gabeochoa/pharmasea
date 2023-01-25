#pragma once

#include "../external_include.h"
//
#include "../engine/keymap.h"
#include "../globals.h"
#include "../map.h"
#include "../toastmanager.h"
#include "../util.h"
#include "internal/channel.h"
#include "steam/steamnetworkingtypes.h"

namespace bitsery {

namespace ext {
template<>
struct PolymorphicBaseClass<BaseComponent>
    : PolymorphicDerivedClasses<
          Transform, HasName, CanHoldItem, SimpleColoredBoxRenderer,
          CanBeHighlighted, CanHighlightOthers, CanHoldFurniture,
          CanBeGhostPlayer, CanPerformJob, ModelRenderer, CanBePushed,
          CanHaveAilment, CustomHeldItemPosition, HasWork, HasBaseSpeed,
          IsSolid, CanBeHeld, IsRotatable, CanGrabFromOtherFurniture,
          ConveysHeldItem, HasWaitingQueue, CanBeTakenFrom,
          IsItemContainer<Bag>, IsItemContainer<PillBottle>, UsesCharacterModel,
          ShowsProgressBar> {};

template<>
struct PolymorphicBaseClass<Item>
    : PolymorphicDerivedClasses<Bag, PillBottle, Pill> {};

template<>
struct PolymorphicBaseClass<LevelInfo>
    : PolymorphicDerivedClasses<LobbyMapInfo, GameMapInfo> {};

}  // namespace ext
}  // namespace bitsery

using MyPolymorphicClasses =
    bitsery::ext::PolymorphicClassesList<Entity, Item, BaseComponent>;

namespace network {

using Buffer = std::string;
using OutputAdapter = bitsery::OutputBufferAdapter<Buffer>;
using InputAdapter = bitsery::InputBufferAdapter<Buffer>;
using TContext =
    std::tuple<bitsery::ext::PointerLinkingContext,
               bitsery::ext::PolymorphicContext<bitsery::ext::StandardRTTI>>;
using BitserySerializer = bitsery::Serializer<OutputAdapter, TContext>;
using BitseryDeserializer = bitsery::Deserializer<InputAdapter, TContext>;

struct ClientPacket {
    Channel channel = Channel::RELIABLE;
    int client_id;

    enum MsgType {
        Announcement,
        Map,
        GameState,
        PlayerControl,
        PlayerJoin,
        PlayerLeave,
        PlayerLocation,
    } msg_type;

    struct AnnouncementInfo {
        std::string message;
        AnnouncementType type;
    };

    // Map Info
    struct MapInfo {
        struct Map map;
    };

    // Game Info
    struct GameStateInfo {
        // TODO we likely dont need to send menu state anymore
        menu::State host_menu_state;
        game::State host_game_state;
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
        size_t hashed_version;
        bool is_you = false;
        std::string username{};
    };

    struct PlayerLeaveInfo {
        std::vector<int> all_clients;
        int client_id = -1;
    };

    // Player Location
    struct PlayerInfo {
        int facing_direction;
        float location[3];
        std::string username{};
    };

    typedef std::variant<
        ClientPacket::AnnouncementInfo, ClientPacket::PlayerControlInfo,
        ClientPacket::PlayerJoinInfo, ClientPacket::GameStateInfo,
        ClientPacket::MapInfo, ClientPacket::PlayerInfo,
        ClientPacket::PlayerLeaveInfo>
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
                return fmt::format("GameStateInfo( menu: {} game: {})",
                                   info.host_menu_state, info.host_game_state);
            },
            [&](ClientPacket::MapInfo) { return std::string("map info"); },
            [&](ClientPacket::PlayerInfo info) {
                return fmt::format("PlayerInfo( pos({}, {}, {}), facing {})",
                                   info.location[0], info.location[1],
                                   info.location[2], info.facing_direction);
            },
            [&](ClientPacket::PlayerLeaveInfo info) {
                return fmt::format("PlayerLeave({})", info.client_id);
            },
            [&](auto) { return std::string(" -- invalid operator<< --"); }},
        msgtype);
    return os;
}

std::ostream& operator<<(std::ostream& os,
                         const ClientPacket::MsgType& msgtype) {
    os << magic_enum::enum_name(msgtype);
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
                  s.value4b(info.type);
              },
              [](S& s, ClientPacket::PlayerJoinInfo& info) {
                  s.container4b(info.all_clients, MAX_CLIENTS);
                  s.value1b(info.is_you);
                  s.value8b(info.hashed_version);
                  s.value4b(info.client_id);
                  s.text1b(info.username, MAX_NAME_LENGTH);
              },
              [](S& s, ClientPacket::PlayerLeaveInfo& info) {
                  s.container4b(info.all_clients, MAX_CLIENTS);
                  s.value4b(info.client_id);
              },
              [](S& s, ClientPacket::PlayerControlInfo& info) {
                  s.container(
                      info.inputs, MAX_INPUTS, [](S& sv, UserInput& input) {
                          sv.ext(
                              input,
                              bitsery::ext::StdTuple{
                                  [](auto& s, menu::State& o) { s.value4b(o); },
                                  [](auto& s, game::State& o) { s.value4b(o); },
                                  [](auto& s, InputName& o) { s.value4b(o); },
                                  [](auto& s, float& o) { s.value4b(o); }});
                      });
              },
              [](S& s, ClientPacket::GameStateInfo& info) {
                  s.value4b(info.host_menu_state);
                  s.value4b(info.host_game_state);
              },
              [](S& s, ClientPacket::MapInfo& info) { s.object(info.map); },
              [](S& s, ClientPacket::PlayerInfo& info) {
                  s.text1b(info.username, MAX_NAME_LENGTH);
                  s.value4b(info.location[0]);
                  s.value4b(info.location[1]);
                  s.value4b(info.location[2]);
                  s.value4b(info.facing_direction);
              },
          });
}

// ClientPacket is in shared.h which is specific to the game,
// TODO how can we support both abstract while also configuration
static ClientPacket deserialize_to_packet(const std::string& msg) {
    TContext ctx{};
    std::get<1>(ctx).registerBasesList<BitseryDeserializer>(
        MyPolymorphicClasses{});

    BitseryDeserializer des{ctx, msg.begin(), msg.size()};

    ClientPacket packet;
    des.object(packet);
    // TODO obviously theres a ton of validation we can do here but idk
    // https://github.com/fraillt/bitsery/blob/master/examples/smart_pointers_with_polymorphism.cpp
    return packet;
}

static Buffer serialize_to_buffer(ClientPacket packet) {
    Buffer buffer;
    TContext ctx{};

    std::get<1>(ctx).registerBasesList<BitserySerializer>(
        MyPolymorphicClasses{});
    BitserySerializer ser{ctx, buffer};
    ser.object(packet);
    ser.adapter().flush();

    return buffer;
}

}  // namespace network
