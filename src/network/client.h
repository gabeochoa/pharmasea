

#pragma once

#include "internal/client.h"
//
#include "../globals_register.h"
#include "../player.h"
#include "../remote_player.h"

namespace network {

struct Client {
    static void log(std::string msg) { std::cout << msg << std::endl; }

    struct ConnectionInfo {
        std::string host_ip_address = "127.0.0.1";
        bool ip_set = false;
    } conn_info;

    int id = 0;
    std::shared_ptr<internal::Client> client_p;
    std::map<int, std::shared_ptr<RemotePlayer>> remote_players;
    std::shared_ptr<Map> map;

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
            // auto player = get_player_packet(my_client_id);
            // client_p->send_packet_to_server(player);

            send_player_input_packet(id);
        }
    }

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
                std::cout << fmt::format("Why are we trying to add {}",
                                         client_id)
                          << std::endl;
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
            std::cout << fmt::format("Adding a player {}", client_id)
                      << std::endl;
        };

        auto remove_player = [&](int client_id) {
            auto rp = remote_players[client_id];
            if (!rp)
                std::cout << fmt::format("doesnt exist but should {}",
                                         client_id)
                          << std::endl;
            rp->cleanup = true;
            remote_players.erase(client_id);
        };

        auto update_remote_player = [&](int client_id, std::string username,
                                        float* location, int facing) {
            if (!remote_players.contains(client_id)) {
                std::cout << fmt::format("doesnt exist but should {}",
                                         client_id)
                          << std::endl;
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
                log(fmt::format("Announcement: {}", info.message));
            } break;
            case ClientPacket::MsgType::PlayerJoin: {
                ClientPacket::PlayerJoinInfo info =
                    std::get<ClientPacket::PlayerJoinInfo>(packet.msg);

                if (info.is_you) {
                    // We are the person that joined,
                    id = info.client_id;
                    log(fmt::format("my id is {}", id));
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
                Menu::get().state = info.host_menu_state;
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
                client_entities_DO_NOT_USE = info.map.entities;
            } break;

            default:
                log(fmt::format("Client: {} not handled yet: {} ",
                                packet.msg_type, msg));
                break;
        }
    }
};

}  // namespace network
