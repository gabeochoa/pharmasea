

#pragma once

#include "shared.h"
//
#include "internal/server.h"
//
#include "../player.h"

namespace network {

struct Server {
    std::shared_ptr<internal::Server> server_p;
    std::map<int, std::shared_ptr<Player> > players;

    explicit Server(int port) {
        server_p.reset(new internal::Server(port));
        server_p->set_process_message(
            std::bind(&Server::server_process_message_string, this,
                      std::placeholders::_1, std::placeholders::_2));
        server_p->startup();
    }

    void tick(float) { server_p->run(); }

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
        ClientPacket packet;
        bitsery::quickDeserialization<InputAdapter>({msg.begin(), msg.size()},
                                                    packet);
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
                        .name = "TODO get name",
                    }),
                });

                server_p->send_client_packet_to_all(player_updated);

            } break;
            case ClientPacket::MsgType::PlayerJoin: {
                // We dont need this
                // ClientPacket::PlayerJoinInfo info =
                // std::get<ClientPacket::PlayerJoinInfo>(packet.msg);
                //

                packet.client_id = incoming_client.client_id;

                if (!players.contains(packet.client_id))
                    players[packet.client_id] = std::make_shared<Player>();

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
