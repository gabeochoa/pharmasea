#pragma once

// This header contains serialization code that requires heavy includes
// Only include this in .cpp files that actually need to serialize/deserialize

#include "../engine/keymap.h"  // for UserInput, UserInputs, InputName, InputSet, InputAmount
#include "../entity.h"
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
    using archive_type = std::remove_cvref_t<decltype(archive)>;
    if (auto result = archive(  //
            info.message,       //
            info.type           //
            );
        zpp::bits::failure(result)) {
        return result;
    }
    if constexpr (archive_type::kind() == zpp::bits::kind::in) {
        if (info.message.size() > MAX_ANNOUNCEMENT_LENGTH) {
            return std::errc::message_size;
        }
    }
    return std::errc{};
}

constexpr auto serialize(auto& archive, ClientPacket::MapInfo& info) {
    return archive(  //
        info.map      //
    );
}

constexpr auto serialize(auto& archive, ClientPacket::MapSeedInfo& info) {
    using archive_type = std::remove_cvref_t<decltype(archive)>;
    if (auto result = archive(  //
            info.seed           //
            );
        zpp::bits::failure(result)) {
        return result;
    }
    if constexpr (archive_type::kind() == zpp::bits::kind::in) {
        if (info.seed.size() > MAX_SEED_LENGTH) {
            return std::errc::message_size;
        }
    }
    return std::errc{};
}

constexpr auto serialize(auto& archive, ClientPacket::GameStateInfo& info) {
    return archive(  //
        info.host_menu_state,  //
        info.host_game_state   //
    );
}

constexpr auto serialize(auto& archive, ClientPacket::PlayerControlInfo& info) {
    using archive_type = std::remove_cvref_t<decltype(archive)>;
    if (auto result = archive(  //
            info.inputs         //
            );
        zpp::bits::failure(result)) {
        return result;
    }
    if constexpr (archive_type::kind() == zpp::bits::kind::in) {
        if (info.inputs.size() > MAX_INPUTS) {
            return std::errc::message_size;
        }
    }
    return std::errc{};
}

constexpr auto serialize(auto& archive, ClientPacket::PlayerJoinInfo& info) {
    using archive_type = std::remove_cvref_t<decltype(archive)>;
    if (auto result = archive(  //
            info.all_clients,   //
            info.client_id,     //
            info.hashed_version, //
            info.is_you,        //
            info.username       //
            );
        zpp::bits::failure(result)) {
        return result;
    }
    if constexpr (archive_type::kind() == zpp::bits::kind::in) {
        if (info.all_clients.size() > MAX_CLIENTS) return std::errc::message_size;
        if (info.username.size() > MAX_NAME_LENGTH) return std::errc::message_size;
    }
    return std::errc{};
}

constexpr auto serialize(auto& archive, ClientPacket::PlayerLeaveInfo& info) {
    using archive_type = std::remove_cvref_t<decltype(archive)>;
    if (auto result = archive(  //
            info.all_clients,   //
            info.client_id      //
            );
        zpp::bits::failure(result)) {
        return result;
    }
    if constexpr (archive_type::kind() == zpp::bits::kind::in) {
        if (info.all_clients.size() > MAX_CLIENTS) return std::errc::message_size;
    }
    return std::errc{};
}

constexpr auto serialize(auto& archive, ClientPacket::PlayerInfo& info) {
    using archive_type = std::remove_cvref_t<decltype(archive)>;
    if (auto result = archive(  //
            info.username,      //
            info.location[0],   //
            info.location[1],   //
            info.location[2],   //
            info.facing         //
            );
        zpp::bits::failure(result)) {
        return result;
    }
    if constexpr (archive_type::kind() == zpp::bits::kind::in) {
        if (info.username.size() > MAX_NAME_LENGTH) return std::errc::message_size;
    }
    return std::errc{};
}

constexpr auto serialize(auto& archive, ClientPacket::PlayerRareInfo& info) {
    return archive(  //
        info.client_id,  //
        info.model_index, //
        info.last_ping    //
    );
}

constexpr auto serialize(auto& archive, ClientPacket::PlaySoundInfo& info) {
    return archive(  //
        info.location[0], //
        info.location[1], //
        info.sound        //
    );
}

constexpr auto serialize(auto& archive, ClientPacket& packet) {
    return archive(  //
        packet.channel,   //
        packet.client_id, //
        packet.msg_type,  //
        packet.msg        //
    );
}

// Serialization function declarations
Buffer serialize_to_entity(Entity* entity);
void deserialize_to_entity(Entity* entity, const std::string& msg);
ClientPacket deserialize_to_packet(const std::string& msg);
Buffer serialize_to_buffer(ClientPacket packet);

}  // namespace network
