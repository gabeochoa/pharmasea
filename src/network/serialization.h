#pragma once

// This header contains serialization code that requires heavy includes
// Only include this in .cpp files that actually need to serialize/deserialize

#include "../bitsery_include.h"
#include "../engine/keymap.h"  // for UserInput, UserInputs, InputName, InputSet, InputAmount
#include "../entity.h"
#include "../world_snapshot_v2.h"
#include "types.h"

namespace network {

// Bitsery type aliases (only needed for serialization)
using OutputAdapter = bitsery::OutputBufferAdapter<Buffer>;
using InputAdapter = bitsery::InputBufferAdapter<Buffer>;
using TContext =
    std::tuple<bitsery::ext::PointerLinkingContext,
               bitsery::ext::PolymorphicContext<bitsery::ext::StandardRTTI>>;
using BitserySerializer = bitsery::Serializer<OutputAdapter, TContext>;
using BitseryDeserializer = bitsery::Deserializer<InputAdapter, TContext>;

// Serialize template for ClientPacket
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
                                      s.container(
                                          o, [](S& sv2, InputAmount& amount) {
                                              sv2.value4b(amount);
                                          });
                                  },
                                  [](auto& s, float& o) { s.value4b(o); }});
                      });
              },
              [](S& s, ClientPacket::GameStateInfo& info) {
                  s.value4b(info.host_menu_state);
                  s.value4b(info.host_game_state);
              },
              [](S& s, ClientPacket::MapInfo& info) {
                  s.object(info.snapshot);
                  s.value1b(info.showMinimap);
              },
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
                  s.value1b(info.sound);
              },
          });
}

// Serialization function declarations
Buffer serialize_to_entity(Entity* entity);
void deserialize_to_entity(Entity* entity, const std::string& msg);
ClientPacket deserialize_to_packet(const std::string& msg);
Buffer serialize_to_buffer(ClientPacket packet);

}  // namespace network
