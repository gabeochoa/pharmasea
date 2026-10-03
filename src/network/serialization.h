#pragma once

// This header contains serialization code that requires heavy includes
// Only include this in .cpp files that actually need to serialize/deserialize

#include "../engine/keymap.h"  // for UserInputSnapshot, UserInputs, InputName, InputPresses, InputAmount
#include "../entities/entity.h"
#include "../map.h"
#include "../zpp_bits_include.h"
#include "types.h"

namespace network {

constexpr auto serialize(auto& archive, ClientPacket::PingInfo& info) {
    return archive(  //
        info.ping,   //
        info.pong    //
    );
}

constexpr auto serialize(auto& archive, ClientPacket::AnnouncementInfo& info) {
    return archive(    //
        info.message,  //
        info.type      //
    );
}

constexpr auto serialize(auto& archive, ClientPacket::MapInfo& info) {
    return archive(  //
        info.map     //
    );
}

constexpr auto serialize(auto& archive, ClientPacket::MapSeedInfo& info) {
    return archive(  //
        info.seed    //
    );
}

constexpr auto serialize(auto& archive, ClientPacket::GameStateInfo& info) {
    return archive(            //
        info.host_menu_state,  //
        info.host_game_state   //
    );
}

constexpr auto serialize(auto& archive, ClientPacket::PlayerControlInfo& info) {
    return archive(  //
        info.inputs  //
    );
}

constexpr auto serialize(auto& archive, ClientPacket::PlayerJoinInfo& info) {
    return archive(           //
        info.all_clients,     //
        info.client_id,       //
        info.hashed_version,  //
        info.is_you,          //
        info.username         //
    );
}

constexpr auto serialize(auto& archive, ClientPacket::PlayerLeaveInfo& info) {
    return archive(        //
        info.all_clients,  //
        info.client_id     //
    );
}

constexpr auto serialize(auto& archive, ClientPacket::PlayerInfo& info) {
    return archive(        //
        info.username,     //
        info.location[0],  //
        info.location[1],  //
        info.location[2],  //
        info.facing        //
    );
}

constexpr auto serialize(auto& archive, ClientPacket::PlayerRareInfo& info) {
    return archive(        //
        info.client_id,    //
        info.model_index,  //
        info.last_ping     //
    );
}

constexpr auto serialize(auto& archive, ClientPacket::PlaySoundInfo& info) {
    return archive(        //
        info.location[0],  //
        info.location[1],  //
        info.sound         //
    );
}

constexpr auto serialize(auto& archive, ClientPacket& packet) {
    return archive(        //
        packet.channel,    //
        packet.client_id,  //
        packet.msg_type,   //
        packet.msg         //
    );
}

// Serialization function declarations
Buffer serialize_to_entity(Entity* entity);
void deserialize_to_entity(Entity* entity, const std::string& msg);
ClientPacket deserialize_to_packet(const std::string& msg);
Buffer serialize_to_buffer(ClientPacket packet);

}  // namespace network
