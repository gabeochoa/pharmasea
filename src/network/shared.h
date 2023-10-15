#pragma once

#include "../external_include.h"
//
#include "../components/adds_ingredient.h"
#include "../components/base_component.h"
#include "../components/can_be_ghost_player.h"
#include "../components/can_be_held.h"
#include "../components/can_be_highlighted.h"
#include "../components/can_be_pushed.h"
#include "../components/can_be_taken_from.h"
#include "../components/can_grab_from_other_furniture.h"
#include "../components/can_highlight_others.h"
#include "../components/can_hold_furniture.h"
#include "../components/can_hold_item.h"
#include "../components/can_order_drink.h"
#include "../components/can_perform_job.h"
#include "../components/collects_user_input.h"
#include "../components/conveys_held_item.h"
#include "../components/custom_item_position.h"
#include "../components/debug_name.h"
#include "../components/has_base_speed.h"
#include "../components/has_client_id.h"
#include "../components/has_dynamic_model_name.h"
#include "../components/has_name.h"
#include "../components/has_patience.h"
#include "../components/has_progression.h"
#include "../components/has_rope_to_item.h"
#include "../components/has_speech_bubble.h"
#include "../components/has_subtype.h"
#include "../components/has_timer.h"
#include "../components/has_waiting_queue.h"
#include "../components/has_work.h"
#include "../components/indexer.h"
#include "../components/is_bank.h"
#include "../components/is_drink.h"
#include "../components/is_item.h"
#include "../components/is_item_container.h"
#include "../components/is_pnumatic_pipe.h"
#include "../components/is_progression_manager.h"
#include "../components/is_rotatable.h"
#include "../components/is_snappable.h"
#include "../components/is_solid.h"
#include "../components/is_spawner.h"
#include "../components/is_trigger_area.h"
#include "../components/model_renderer.h"
#include "../components/responds_to_user_input.h"
#include "../components/shows_progress_bar.h"
#include "../components/simple_colored_box_renderer.h"
#include "../components/transform.h"
#include "../components/uses_character_model.h"

//

#include "../engine/keymap.h"
#include "../engine/time.h"
#include "../engine/toastmanager.h"
#include "../engine/util.h"
#include "../globals.h"
#include "../job.h"
#include "../map.h"
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
          CustomHeldItemPosition, HasWork, HasBaseSpeed, IsSolid, CanBeHeld,
          HasPatience, HasProgression, IsRotatable, CanGrabFromOtherFurniture,
          ConveysHeldItem, HasWaitingQueue, CanBeTakenFrom, IsItemContainer,
          UsesCharacterModel, ShowsProgressBar, DebugName, HasDynamicModelName,
          IsTriggerArea, HasSpeechBubble, Indexer, IsSpawner, HasTimer,
          HasRopeToItem, HasSubtype, IsItem, IsDrink, AddsIngredient,
          CanOrderDrink, IsPnumaticPipe, IsProgressionManager, IsFloorMarker,
          IsBank,

          // This one i didnt find before cause its not serialzed generally
          CollectsUserInput> {};
// If you add anything here ^^ then you should add that component to
// register_all_components in entity.h

template<>
struct PolymorphicBaseClass<Job>
    : PolymorphicDerivedClasses<WaitJob, WanderingJob, WaitInQueueJob,
                                LeavingJob> {};

}  // namespace ext
}  // namespace bitsery

using MyPolymorphicClasses =
    bitsery::ext::PolymorphicClassesList<BaseComponent, Job>;

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

inline std::ostream& operator<<(std::ostream& os,
                                const ClientPacket::Msg& msgtype) {
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
            [&](ClientPacket::MapSeedInfo s) {
                return fmt::format("Map Seed: {}", s.seed);
            },
            [&](ClientPacket::PlayerInfo info) {
                return fmt::format("PlayerInfo( pos({}, {}, {}), facing {})",
                                   info.location[0], info.location[1],
                                   info.location[2], info.facing);
            },
            [&](ClientPacket::PlayerLeaveInfo info) {
                return fmt::format("PlayerLeave({})", info.client_id);
            },
            [&](ClientPacket::PlayerRareInfo info) {
                return fmt::format("PlayerRare({})", info.client_id);
            },
            [&](ClientPacket::PingInfo info) {
                return fmt::format("Ping({}, {})", info.ping, info.pong);
            },
            [&](ClientPacket::PlaySoundInfo info) {
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
                                  [](auto& s, InputSet& o) {
                                      s.ext(o, bitsery::ext::StdBitset{});
                                  },
                                  [](auto& s, float& o) { s.value4b(o); }});
                      });
              },
              [](S& s, ClientPacket::GameStateInfo& info) {
                  s.value4b(info.host_menu_state);
                  s.value4b(info.host_game_state);
              },
              [](S& s, ClientPacket::MapInfo& info) { s.object(info.map); },
              [](S& s, ClientPacket::MapSeedInfo& info) {
                  s.text1b(info.seed, MAX_SEED_LENGTH);
              },
              [](S& s, ClientPacket::PlayerInfo& info) {
                  s.text1b(info.username, MAX_NAME_LENGTH);
                  s.value4b(info.location[0]);
                  s.value4b(info.location[1]);
                  s.value4b(info.location[2]);
                  s.value4b(info.facing);
              },
              [](S& s, ClientPacket::PlayerRareInfo& info) {
                  s.value4b(info.client_id);
                  s.value4b(info.model_index);
                  s.value8b(info.last_ping);
              },
              [](S& s, ClientPacket::PingInfo& info) {
                  s.value8b(info.ping);
                  s.value8b(info.pong);
              },
              [](S& s, ClientPacket::PlaySoundInfo& info) {
                  s.value4b(info.location[0]);
                  s.value4b(info.location[1]);
                  s.text1b(info.sound, network::MAX_SOUND_LENGTH);
              },
          });
}

static Buffer serialize_to_entity(Entity* entity) {
    Buffer buffer;
    TContext ctx{};

    std::get<1>(ctx).registerBasesList<BitserySerializer>(
        MyPolymorphicClasses{});
    BitserySerializer ser{ctx, buffer};
    ser.object(*entity);
    ser.adapter().flush();

    return buffer;
}

static void deserialize_to_entity(Entity* entity, const std::string& msg) {
    TContext ctx{};
    std::get<1>(ctx).registerBasesList<BitseryDeserializer>(
        MyPolymorphicClasses{});

    BitseryDeserializer des{ctx, msg.begin(), msg.size()};
    des.object(*entity);

    switch (des.adapter().error()) {
        case bitsery::ReaderError::NoError:
            break;
        case bitsery::ReaderError::ReadingError:
            log_error("reading error");
            break;
        case bitsery::ReaderError::DataOverflow:
            log_error("data overflow error");
            break;
        case bitsery::ReaderError::InvalidData:
            log_error("invalid data error");
            break;
        case bitsery::ReaderError::InvalidPointer:
            log_error("invalid pointer error");
            break;
    }
    assert(des.adapter().error() == bitsery::ReaderError::NoError);
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
