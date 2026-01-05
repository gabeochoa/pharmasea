

#include "client.h"

#include "../building_locations.h"
#include "../components/collects_user_input.h"
#include "../components/has_client_id.h"
#include "../components/has_name.h"
#include "../engine/globals_register.h"
#include "../engine/log.h"
#include "../engine/sound_library.h"
#include "../engine/time.h"
#include "../engine/toastmanager.h"
#include "../post_deserialize_fixups.h"
#include "network.h"
#include "serialization.h"

namespace network {

Client::Client() {
    // Ensure Afterhours' default collection matches the client thread's
    // collection. (Some serialization paths call afterhours::EntityHelper
    // directly.)
    afterhours::EntityHelper::set_default_collection(
        &EntityHelper::get_client_collection());

    client_p = std::make_unique<internal::Client>();
    client_p->set_process_message([this](const std::string& msg) {
        this->client_process_message_string(msg);
    });

    map = std::make_unique<Map>("default_seed");
    GLOBALS.set("map", map.get());
}

void Client::update_username(const std::string& new_name) {
    client_p->username = new_name;
}

void Client::lock_in_ip() {
    conn_info.ip_set = true;
    client_p->set_address(conn_info.host_ip_address);
    client_p->startup();
}

void Client::tick(float dt) {
    next_tick = next_tick - dt;

    client_p->run();
    if (next_tick > 0) return;
    next_tick = next_tick_reset;

    //

    if (id > 0) {
        send_ping_packet(id, dt);
        send_player_input_packet(id);
    }

    if (client_p->is_not_connected()) {
        announcements.push_back({
            .message = "Lost connection to Host",
            .type = AnnouncementType::Error,
        });
        MenuState::get().reset();
        GameState::get().reset();
    }
}

void Client::send_ping_packet(int my_id, float dt) {
    next_ping = next_ping - dt;
    if (next_ping > 0) return;
    next_ping = next_ping_reset;

    ClientPacket packet{
        .channel = Channel::UNRELIABLE_NO_DELAY,
        .client_id = my_id,
        .msg_type = network::ClientPacket::MsgType::Ping,
        .msg = network::ClientPacket::PingInfo({
            .ping = now::current_ms(),
            .pong = now::current_ms(),
        }),
    };
    send_packet_to_server(packet);
}

// TODO :DUPE: this is duplicated with the version in internal/client
void Client::send_packet_to_server(ClientPacket packet) {
    Buffer buffer = serialize_to_buffer(packet);
    client_p->send_string_to_server(buffer, packet.channel);
}

void Client::send_player_input_packet(int my_id) {
    // TODO if we add support for local players we need them all to have
    // their own unique network ids otherwise we cant distinguish when
    // sending player updates
    CollectsUserInput& cui =
        map->local_players_NOT_SERIALIZED[0]->get<CollectsUserInput>();

    if (cui.empty()) return;

    ClientPacket packet{
        .channel = Channel::UNRELIABLE_NO_DELAY,
        .client_id = my_id,
        .msg_type = network::ClientPacket::MsgType::PlayerControl,
        .msg = network::ClientPacket::PlayerControlInfo({
            .inputs = cui.inputs_NETWORK_ONLY(),
        }),
    };
    cui.clear();
    send_packet_to_server(packet);
}

void Client::send_current_menu_state() {
    ClientPacket packet({
        .client_id = SERVER_CLIENT_ID,
        .msg_type = ClientPacket::MsgType::GameState,
        .msg = ClientPacket::GameStateInfo({
            .host_menu_state = MenuState::get().read(),
            .host_game_state = GameState::get().read(),
        }),
    });
    Server::forward_packet(packet);
}

void Client::send_updated_seed(const std::string& seed) {
    ClientPacket packet{
        .client_id = SERVER_CLIENT_ID,
        .msg_type = ClientPacket::MsgType::MapSeed,
        .msg = ClientPacket::MapSeedInfo{.seed = seed},
    };

    Server::queue_packet(packet);
}

void Client::client_process_message_string(const std::string& msg) {
    auto add_new_player = [&](int client_id, const std::string& username) {
        if (remote_players.contains(client_id)) {
            log_warn("Why are we trying to add {}", client_id);
            return;
        };

        // Note: the reason we use new and not createEntity is that we dont
        // want this in the array that is serialized, this should only live
        // in remote_players
        Entity* entity = new Entity();
        make_remote_player(*entity, LOBBY_BUILDING.to3());
        remote_players[client_id] = std::shared_ptr<Entity>(entity);
        const auto& rp = remote_players[client_id];
        rp->get<HasClientID>().update(client_id);
        // We want to crash if no hasName so no has<> check here
        rp->get<HasName>().update(username);
        // NOTE we add to the map directly because its colocated with
        //      the other entity info

        map->remote_players_NOT_SERIALIZED.push_back(remote_players[client_id]);
        log_info("Adding a player {}", client_id);
    };

    auto remove_player = [&](int client_id) {
        for (auto it = map->remote_players_NOT_SERIALIZED.begin();
             it != map->remote_players_NOT_SERIALIZED.end(); it++) {
            if ((*it)->get<HasClientID>().id() == client_id) {
                map->remote_players_NOT_SERIALIZED.erase(it);
                break;
            }
        }

        auto rp_it = remote_players.find(client_id);
        if (rp_it == remote_players.end()) {
            log_warn("RemovePlayer:: Remote player doesnt exist but should: {}",
                     client_id);
            return;
        }

        auto rp = rp_it->second;
        if (!rp) {
            log_warn(
                "RemovePlayer:: Remote player exists but has null "
                "shared_ptr",
                client_id);
        } else {
            rp->cleanup = true;
        }

        remote_players.erase(client_id);
    };

    auto update_remote_player = [&](int client_id, std::string username,
                                    float* location, float facing) {
        if (!remote_players.contains(client_id)) {
            log_warn("Remote player doesnt exist but should: {}", client_id);
            add_new_player(client_id, username);
        }
        auto rp = remote_players[client_id];
        if (!rp) return;
        update_player_remotely(*rp, location, username, facing);
    };

    auto update_remote_player_rare = [&](int client_id,    //
                                         int model_index,  //
                                         long long last_ping) {
        if (!remote_players.contains(client_id)) {
            log_warn("(rare) Remote player doesnt exist but should: {}",
                     client_id);
            return;
        }
        auto rp = remote_players[client_id];
        if (!rp) {
            log_warn("remote player {} is not valid - rare update", client_id);
        }

        // log_info("updating remote player arre {} {} id{}", client_id,
        // model_index, rp->id);
        update_player_rare_remotely(*rp, model_index, last_ping);
    };

    ClientPacket packet = deserialize_to_packet(msg);

    // log_info("Client: recieved packet {}", packet.msg_type);

    switch (packet.msg_type) {
        case ClientPacket::MsgType::Announcement: {
            ClientPacket::AnnouncementInfo info =
                std::get<ClientPacket::AnnouncementInfo>(packet.msg);
            announcements.push_back(info);
            // Also surface immediately as an in-game toast.
            TOASTS.push_back(
                {.msg = info.message, .type = info.type, .timeToShow = 5});
        } break;
        case ClientPacket::MsgType::PlayerLeave: {
            ClientPacket::PlayerLeaveInfo info =
                std::get<ClientPacket::PlayerLeaveInfo>(packet.msg);
            remove_player(info.client_id);
        } break;
        case ClientPacket::MsgType::PlayerJoin: {
            ClientPacket::PlayerJoinInfo info =
                std::get<ClientPacket::PlayerJoinInfo>(packet.msg);

            if (info.is_you) {
                // We are the person that joined,
                id = info.client_id;
                log_info("my id is {}", id);
                add_new_player(id, client_p->username);
                GLOBALS.set("active_camera_target", remote_players[id].get());
                map->local_players_NOT_SERIALIZED.push_back(remote_players[id]);
                (*(map->local_players_NOT_SERIALIZED.rbegin()))
                    ->addComponent<CollectsUserInput>();
                (*(map->local_players_NOT_SERIALIZED.rbegin()))
                    ->addComponent<CanBeGhostPlayer>();
            }

            for (auto client_id : info.all_clients) {
                if (info.is_you && client_id == info.client_id) continue;
                // otherwise someone just joined and we have to deal
                // with them
                add_new_player(client_id, info.username);
            }

        } break;
        case ClientPacket::MsgType::GameState: {
            ClientPacket::GameStateInfo info =
                std::get<ClientPacket::GameStateInfo>(packet.msg);
            // TODO do we need to clear?
            // Menu::get().clear_history();

            // We actually only need to do this one
            if (MenuState::get().read() == menu::State::Network &&
                info.host_menu_state == menu::State::Game) {
                MenuState::get().set(info.host_menu_state);
            }
            if (GameState::get().is_not(game::State::Paused)) {
                GameState::get().set(info.host_game_state);
            }

        } break;
        case ClientPacket::MsgType::PlayerLocation: {
            ClientPacket::PlayerInfo info =
                std::get<ClientPacket::PlayerInfo>(packet.msg);
            update_remote_player(packet.client_id, info.username, info.location,
                                 info.facing);
        } break;
        case ClientPacket::MsgType::Map: {
            ClientPacket::MapInfo info =
                std::get<ClientPacket::MapInfo>(packet.msg);

            post_deserialize_fixups::run();

            map->update_map(info.map);

        } break;

        case ClientPacket::MsgType::PlayerRare: {
            ClientPacket::PlayerRareInfo info =
                std::get<ClientPacket::PlayerRareInfo>(packet.msg);
            update_remote_player_rare(info.client_id, info.model_index,
                                      info.last_ping);
        } break;

        case ClientPacket::MsgType::Ping: {
            ClientPacket::PingInfo info =
                std::get<ClientPacket::PingInfo>(packet.msg);

            auto pong = now::current_ms();

            there_ping = info.pong - info.ping;
            return_ping = pong - info.pong;
            total_ping = pong - info.ping;
        } break;

        case ClientPacket::MsgType::PlaySound: {
            ClientPacket::PlaySoundInfo info =
                std::get<ClientPacket::PlaySoundInfo>(packet.msg);

            // TODO find a way to play a sound at a position?
            // vec2 _pos = {
            // info.location[0],
            // info.location[1],
            // };

            SoundLibrary::get().play(strings::sounds::to_name(info.sound));

        } break;

        default:
            log_warn("Client: {} not handled yet: {} ", packet.msg_type, msg);
            break;
    }
}

}  // namespace network
