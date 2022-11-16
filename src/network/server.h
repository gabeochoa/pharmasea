

#pragma once

#include <thread>
//
#include "../atomic_queue.h"
#include "shared.h"
//
#include "internal/server.h"
//
#include "../map.h"
#include "../player.h"

namespace network {

// no singleton.h here because we dont want this publically announced
struct Server;
static std::shared_ptr<Server> g_server;

struct Server {
    static std::thread start(int port) {
        g_server.reset(new Server(port));
        return std::thread([&] {
            g_server->running = true;
            g_server->run();
        });
    }

    static void queue_packet(ClientPacket& p) {
        g_server->packet_queue.push_back(p);
    }

    static void stop() { g_server->running = false; }

   private:
    AtomicQueue<ClientPacket> packet_queue;
    std::shared_ptr<internal::Server> server_p;
    std::map<int, std::shared_ptr<Player> > players;
    std::shared_ptr<Map> pharmacy_map;
    std::atomic<bool> running;
    std::thread::id thread_id;
    Menu::State current_state;

    explicit Server(int port) {
        server_p.reset(new internal::Server(port));
        server_p->set_process_message(
            std::bind(&Server::server_process_message_string, this,
                      std::placeholders::_1, std::placeholders::_2));
        server_p->startup();

        // TODO add some kind of seed selection screen

        pharmacy_map.reset(new Map());
        GLOBALS.set("server_map", pharmacy_map.get());
    }

    void send_map_state() {
        pharmacy_map->grab_things();
        pharmacy_map->ensure_generated_map();

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

    void run() {
        thread_id = std::this_thread::get_id();
        GLOBALS.set("server_thread_id", &thread_id);

        using namespace std::chrono_literals;
        auto start = std::chrono::high_resolution_clock::now();
        auto end = start;
        float duration = 0.f;
        while (running) {
            tick(duration);

            do {
                end = std::chrono::high_resolution_clock::now();
                duration =
                    std::chrono::duration_cast<std::chrono::milliseconds>(end -
                                                                          start)
                        .count();
                std::this_thread::sleep_for(1ms);
            } while (duration < 4);

            start = end;
        }
    }

    // NOTE: server time things are in s
    float next_map_tick_reset = 100;  // 1.20fps
    float next_map_tick = 0;

    float next_update_tick_reset = 4;  // 30fps
    float next_update_tick = 0;

    void tick(float dt) {
        server_p->run();

        // Check to see if we have any packets to send off
        while (!packet_queue.empty()) {
            ClientPacket& p = packet_queue.front();

            switch (p.msg_type) {
                case ClientPacket::MsgType::GameState: {
                    ClientPacket::GameStateInfo info =
                        std::get<ClientPacket::GameStateInfo>(p.msg);
                    current_state = info.host_menu_state;
                } break;
                default:
                    break;
            }

            //
            server_p->send_client_packet_to_all(p);
            packet_queue.pop_front();
        }

        if (Menu::in_game(current_state)) {
            next_update_tick += dt;
            if (next_update_tick >= next_update_tick_reset) {
                for (auto p : players) {
                    p.second->update(next_update_tick / 1000.f);
                }

                if (current_state == Menu::State::Game) {
                    pharmacy_map->onUpdate(next_update_tick / 1000.f);
                }
                next_update_tick = 0.f;
            }
        }

        next_map_tick -= dt;
        if (next_map_tick <= 0) {
            send_map_state();
            next_map_tick = next_map_tick_reset;
        }
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

                if (info.hashed_version != HASHED_VERSION) {
                    // TODO send error message
                    std::cout
                        << "player tried to join but had incorrect version"
                        << " our version: " << HASHED_VERSION
                        << " their version: " << info.hashed_version
                        << std::endl;
                    return;
                }

                // overwrite it so its already there
                packet.client_id = incoming_client.client_id;

                // create the player if they dont already exist
                if (!players.contains(packet.client_id)) {
                    players[packet.client_id] = std::make_shared<Player>();
                }

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
