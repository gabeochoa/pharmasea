

#pragma once

#include "internal/client.h"
//
#include "../engine/globals_register.h"
#include "../engine/log.h"
#include "../player.h"
#include "../remote_player.h"

namespace network {

struct Client {
    struct ConnectionInfo {
        std::string host_ip_address = "127.0.0.1";
        bool ip_set = false;
    } conn_info;

    int id = 0;
    std::shared_ptr<internal::Client> client_p;
    std::map<int, std::shared_ptr<RemotePlayer>> remote_players;
    std::shared_ptr<Map> map;
    std::vector<ClientPacket::AnnouncementInfo> announcements;

    float next_tick_reset = 0.02f;
    float next_tick = 0.0f;

    Client() {
        client_p.reset(new internal::Client());
        client_p->set_process_message(
            std::bind(&Client::client_process_message_string, this,
                      std::placeholders::_1));

        map.reset(new Map());
        GLOBALS.set("map", map.get());
    }

    void update_username(std::string new_name) {
        client_p->username = new_name;
    }

    void lock_in_ip() {
        conn_info.ip_set = true;
        client_p->set_address(conn_info.host_ip_address);
        client_p->startup();
    }

    void tick(float dt) {
        next_tick = next_tick - dt;

        client_p->run();
        if (next_tick > 0) {
            return;
        }

        next_tick = next_tick_reset;
        if (id > 0) {
            // TODO figure out why this code was commented out
            // auto player = get_player_packet(my_client_id);
            // client_p->send_packet_to_server(player);

            send_player_input_packet(id);
        }

        if (!client_p->is_connected()) {
            announcements.push_back({
                .message = "Lost connection to Host",
                .type = AnnouncementType::Error,
            });
            MenuState::get().reset();
            GameState::get().reset();
        }
    }

    // TODO figure out why this code was commented out
    // ClientPacket get_player_packet(int my_id) {
    // Player me = GLOBALS.get<Player>("player");
    // ClientPacket player({
    // .channel = Channel::UNRELIABLE_NO_DELAY,
    // .client_id = my_id,
    // .msg_type = network::ClientPacket::MsgType::PlayerLocation,
    // .msg = network::ClientPacket::PlayerInfo({
    // .facing_direction = static_cast<int>(me.face_direction),
    // .location =
    // {
    // me.position.x,
    // me.position.y,
    // me.position.z,
    // },
    // .name = Settings::get().data.username,
    // }),
    // });
    // return player;
    // }

    void send_player_input_packet(int my_id) {
        Player* me = GLOBALS.get_ptr<Player>("player");

        if (me->inputs.empty()) return;

        ClientPacket packet({
            .channel = Channel::UNRELIABLE_NO_DELAY,
            .client_id = my_id,
            .msg_type = network::ClientPacket::MsgType::PlayerControl,
            .msg = network::ClientPacket::PlayerControlInfo({
                .inputs = me->inputs,
            }),
        });
        me->inputs.clear();
        client_p->send_packet_to_server(packet);
    }

    void client_process_message_string(std::string msg) {
        auto add_new_player = [&](int client_id, std::string username) {
            if (remote_players.contains(client_id)) {
                log_warn("Why are we trying to add {}", client_id);
                return;
            };

            remote_players[client_id] = std::make_shared<RemotePlayer>();
            auto rp = remote_players[client_id];
            rp->client_id = client_id;
            rp->update_name(username);
            // NOTE we add to the map directly because its colocated with
            //      the other entity info
            map->remote_players_NOT_SERIALIZED.push_back(
                remote_players[client_id]);
            log_info("Adding a player {}", client_id);
        };

        auto remove_player = [&](int client_id) {
            for (auto it = map->remote_players_NOT_SERIALIZED.begin();
                 it != map->remote_players_NOT_SERIALIZED.end(); it++) {
                if ((*it)->client_id == client_id) {
                    map->remote_players_NOT_SERIALIZED.erase(it);
                    break;
                }
            }

            auto rp_it = remote_players.find(client_id);
            if (rp_it == remote_players.end()) {
                log_warn(
                    "RemovePlayer:: Remote player doesnt exist but should: {}",
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
                                        float* location, int facing) {
            if (!remote_players.contains(client_id)) {
                log_warn("Remote player doesnt exist but should: {}",
                         client_id);
                add_new_player(client_id, username);
            }
            auto rp = remote_players[client_id];
            if (!rp) return;
            rp->update_remotely(location, username, facing);
        };

        ClientPacket packet = client_p->deserialize_to_packet(msg);

        switch (packet.msg_type) {
            case ClientPacket::MsgType::Announcement: {
                ClientPacket::AnnouncementInfo info =
                    std::get<ClientPacket::AnnouncementInfo>(packet.msg);
                announcements.push_back(info);
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
                    GLOBALS.set("active_camera_target",
                                remote_players[id].get());
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
                MenuState::get().set(info.host_menu_state);
                GameState::get().set(info.host_game_state);
            } break;
            case ClientPacket::MsgType::PlayerLocation: {
                ClientPacket::PlayerInfo info =
                    std::get<ClientPacket::PlayerInfo>(packet.msg);
                update_remote_player(packet.client_id, info.username,
                                     info.location, info.facing_direction);
            } break;
            case ClientPacket::MsgType::Map: {
                ClientPacket::MapInfo info =
                    std::get<ClientPacket::MapInfo>(packet.msg);
                client_entities_DO_NOT_USE = info.map.entities();
                client_items_DO_NOT_USE = info.map.items();
            } break;

            default:
                log_warn("Client: {} not handled yet: {} ", packet.msg_type,
                         msg);
                break;
        }
    }
};

}  // namespace network
