

#pragma once

#include "shared.h"
//
#include "internal/server.h"
//
#include "../map.h"
#include "../player.h"

namespace network {

struct Server {
    std::shared_ptr<internal::Server> server_p;
    std::map<int, std::shared_ptr<Player> > players;
    std::shared_ptr<Map> pharmacy_map;

    float next_map_tick_reset = 0.04f;
    float next_map_tick = 0.0f;

    explicit Server(int port) {
        server_p.reset(new internal::Server(port));
        server_p->set_process_message(
            std::bind(&Server::server_process_message_string, this,
                      std::placeholders::_1, std::placeholders::_2));
        server_p->startup();

        // TODO add some kind of seed selection screen

        pharmacy_map.reset(new Map());
    }

    void tick(float) {
        server_p->run();

        {
            if (next_map_tick > 0) {
                return;
            }
            next_map_tick = next_map_tick_reset;
            send_map_state();
        }
    }

    void send_map_state() {
        ClientPacket map_packet({
            .channel = Channel::RELIABLE,
            .client_id = SERVER_CLIENT_ID,
            .msg_type = network::ClientPacket::MsgType::Map,
            .msg = network::ClientPacket::MapInfo({
                .map = *pharmacy_map,
            }),
        });
        server_p->send_client_packet_to_all(map_packet);
    }

    void send_menu_state(Menu::State state) {
        ClientPacket player({
            .client_id = SERVER_CLIENT_ID,
            .msg_type = ClientPacket::MsgType::GameState,
            .msg = ClientPacket::GameStateInfo({.host_menu_state = state}),
        });
        server_p->send_client_packet_to_all(player);
    }

    void server_process_message_string(const Client_t& incoming_client,
                                       std::string msg) {
        ClientPacket packet = server_p->deserialize_to_packet(msg);

        switch (packet.msg_type) {
            case ClientPacket::MsgType::Announcement: {
                // TODO send announcements to all clients
                ClientPacket::AnnouncementInfo info =
                    std::get<ClientPacket::AnnouncementInfo>(packet.msg);
            } break;

            case ClientPacket::MsgType::PlayerControl: {
                ClientPacket::PlayerControlInfo info =
                    std::get<ClientPacket::PlayerControlInfo>(packet.msg);

                auto player = players[packet.client_id];

                if (!player) return;

                auto updated_position =
                    player->get_position_after_input(info.inputs);

                // TODO if the position and face direction didnt change
                //      then we can early return
                //
                // NOTE: i saw issues where == between vec3 was returning
                //      on every call because (mvt * dt) < epsilon
                //

                ClientPacket player_updated({
                    .channel = Channel::UNRELIABLE_NO_DELAY,
                    .client_id = incoming_client.client_id,
                    .msg_type = network::ClientPacket::MsgType::PlayerLocation,
                    .msg = network::ClientPacket::PlayerInfo({
                        .facing_direction =
                            static_cast<int>(player->face_direction),
                        .location =
                            {
                                updated_position.x,
                                updated_position.y,
                                updated_position.z,
                            },
                        .username = player->username,
                    }),
                });

                server_p->send_client_packet_to_all(player_updated);

            } break;
            case ClientPacket::MsgType::PlayerJoin: {
                ClientPacket::PlayerJoinInfo info =
                    std::get<ClientPacket::PlayerJoinInfo>(packet.msg);

                // overwrite it so its already there
                packet.client_id = incoming_client.client_id;

                // create the player if they dont already exist
                if (!players.contains(packet.client_id))
                    players[packet.client_id] = std::make_shared<Player>();

                // update the username
                players[packet.client_id]->username = info.username;

                std::vector<int> ids;
                for (auto& c : server_p->clients) {
                    ids.push_back(c.second.client_id);
                }
                // Since we are the host, we can use the Client_t to figure
                // out the id / name
                server_p->send_client_packet_to_all(
                    ClientPacket(
                        {.client_id = SERVER_CLIENT_ID,
                         .msg_type = ClientPacket::MsgType::PlayerJoin,
                         .msg = ClientPacket::PlayerJoinInfo({
                             .all_clients = ids,
                             // override the client's id with their real one
                             .client_id = incoming_client.client_id,
                             .is_you = false,
                         })}),
                    // ignore the person who sent it to us
                    [&](Client_t& client) {
                        return client.client_id == incoming_client.client_id;
                    });

                server_p->send_client_packet_to_all(
                    ClientPacket(
                        {.client_id = SERVER_CLIENT_ID,
                         .msg_type = ClientPacket::MsgType::PlayerJoin,
                         .msg = ClientPacket::PlayerJoinInfo({
                             .all_clients = ids,
                             // override the client's id with their real one
                             .client_id = incoming_client.client_id,
                             .is_you = true,
                         })}),
                    // ignore everyone except the one that sent to us
                    [&](Client_t& client) {
                        return client.client_id != incoming_client.client_id;
                    });
            } break;
            default:
                server_p->send_client_packet_to_all(
                    packet, [&](Client_t& client) {
                        return client.client_id == incoming_client.client_id;
                    });
                // log(fmt::format("Server: {} not handled yet ",
                // packet.msg_type));
                break;
        }
    }
};

}  // namespace network
