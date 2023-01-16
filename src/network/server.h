

#pragma once

#include <thread>
//
#include "../engine/atomic_queue.h"
#include "../engine/tracy.h"
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
    // TODO once clang supports jthread replace with jthread and remove "running
    // = true" to use stop_token
    static std::thread start(int port) {
        g_server.reset(new Server(port));
        g_server->running = true;
        return std::thread(std::bind(&Server::run, g_server.get()));
    }

    static void queue_packet(ClientPacket& p) {
        g_server->packet_queue.push_back(p);
    }

    static void stop() { g_server->running = false; }

   private:
    typedef std::pair<Client_t, std::string> ClientMessage;
    AtomicQueue<ClientMessage> incoming_message_queue;
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
            std::bind(&Server::server_enqueue_message_string, this,
                      std::placeholders::_1, std::placeholders::_2));
        server_p->onClientDisconnect = std::bind(&Server::process_player_leave,
                                                 this, std::placeholders::_1);
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
        TRACY_ZONE_SCOPED;
        thread_id = std::this_thread::get_id();
        GLOBALS.set("server_thread_id", &thread_id);

        using namespace std::chrono_literals;
        auto start = std::chrono::high_resolution_clock::now();
        auto end = start;
        float duration = 0.f;
        while (running) {
            tick(duration);

            do {
                TRACY_ZONE_NAMED(server_run_wait_loop,
                                 "waiting for run loop continue", true);
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

    // TODO verify that these numbers make sense, i have a feeling
    // its not 2fps but 1/50 seconds which woudl be 0.5fps
    // NOTE: server time things are in s
    float next_map_tick_reset = 50;  // 2fps
    float next_map_tick = 0;

    float next_update_tick_reset = 4;  // 30fps
    float next_update_tick = 0;

    void tick(float dt) {
        TRACY_ZONE_SCOPED;
        server_p->run();

        // Check to see if we have any new packets to process
        while (!incoming_message_queue.empty()) {
            TRACY_ZONE_NAMED(tracy_server_process, "packets to process", true);
            log_trace("Incoming Messages {}", incoming_message_queue.size());
            server_process_message_string(incoming_message_queue.front());
            incoming_message_queue.pop_front();
        }

        // TODO move one of these while loops, probably the one belowVVV to a
        // different thread
        // This will allow us to empty the steam queue faster, and give us more
        // control over which messages to drop

        // Check to see if we have any packets to send off
        while (!packet_queue.empty()) {
            TRACY_ZONE_NAMED(tracy_server_fwd, "packets to fwd", true);
            log_trace("Packets to FWD {}", packet_queue.size());
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
            TRACY_ZONE_NAMED(tracy_server_gametick, "process game tick", true);
            next_update_tick += dt;
            if (next_update_tick >= next_update_tick_reset) {
                for (auto p : players) {
                    p.second->update(next_update_tick / 1000.f);
                }

                pharmacy_map->onUpdate(next_update_tick / 1000.f);
                next_update_tick = 0.f;
            }
        }

        next_map_tick -= dt;
        if (next_map_tick <= 0) {
            send_map_state();
            next_map_tick = next_map_tick_reset;
        }
        TRACY_FRAME_MARK("server::tick");
    }

    void process_announcement_packet(const Client_t&,
                                     const ClientPacket& packet) {
        const ClientPacket::AnnouncementInfo info =
            std::get<ClientPacket::AnnouncementInfo>(packet.msg);
        server_p->send_client_packet_to_all(packet);
    }

    void process_player_control_packet(const Client_t& incoming_client,
                                       const ClientPacket& packet) {
        const ClientPacket::PlayerControlInfo info =
            std::get<ClientPacket::PlayerControlInfo>(packet.msg);

        auto player = players[packet.client_id];

        if (!player) return;

        auto updated_position = player->get_position_after_input(info.inputs);

        // TODO if the position and face direction didnt change
        //      then we can early return
        //
        // NOTE: i saw issues where == between vec3 was returning
        //      on every call because (mvt * dt) < epsilon
        //

        ClientPacket player_updated({
            .channel = Channel::UNRELIABLE,
            .client_id = incoming_client.client_id,
            .msg_type = network::ClientPacket::MsgType::PlayerLocation,
            .msg = network::ClientPacket::PlayerInfo({
                .facing_direction = static_cast<int>(player->face_direction),
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
    }

    void process_player_leave(int client_id) {
        log_info("processing player leave for {}", client_id);
        auto player_match = players.find(client_id);
        if (player_match == players.end()) {
            log_warn("We dont have a matching player for {}", client_id);
            return;
        }
        // TODO We might have to force them to drop everything or something?
        players.erase(player_match);

        std::vector<int> ids;
        for (auto& c : server_p->clients) {
            ids.push_back(c.second.client_id);
        }
        // Since we are the host, we can use the Client_t to figure
        // out the id / name
        server_p->send_client_packet_to_all(
            ClientPacket({.client_id = SERVER_CLIENT_ID,
                          .msg_type = ClientPacket::MsgType::PlayerLeave,
                          .msg = ClientPacket::PlayerLeaveInfo({
                              .all_clients = ids,
                              // override the client's id with their real one
                              .client_id = client_id,
                          })}),
            // ignore the person who sent it to us since they disconn
            [&](Client_t& client) { return client.client_id == client_id; });
    }

    void process_player_leave_packet(const Client_t& incoming_client,
                                     const ClientPacket&) {
        log_info("processing player leave packet for {}",
                 incoming_client.client_id);
        // ClientPacket::PlayerLeaveInfo info =
        // std::get<ClientPacket::PlayerLeaveInfo>(orig_packet.msg);
        process_player_leave(incoming_client.client_id);
    }

    void process_player_join_packet(const Client_t& incoming_client,
                                    const ClientPacket& orig_packet) {
        ClientPacket::PlayerJoinInfo info =
            std::get<ClientPacket::PlayerJoinInfo>(orig_packet.msg);

        if (info.hashed_version != HASHED_VERSION) {
            // TODO send error message announcement
            log_warn(
                "player tried to join but had incorrect version our "
                "version : {}, their version : {} ",
                HASHED_VERSION, info.hashed_version);
            return;
        }

        ClientPacket packet(orig_packet);
        // overwrite it so its already there
        packet.client_id = incoming_client.client_id;

        // create the player if they dont already exist
        if (!players.contains(packet.client_id)) {
            players[packet.client_id] = std::make_shared<Player>();
        }

        // update the username
        players[packet.client_id]->username = info.username;

        // TODO i looked into std::transform but kept getting std::out of range
        // errors
        std::vector<int> ids;
        for (auto& c : server_p->clients) {
            ids.push_back(c.second.client_id);
        }
        // Since we are the host, we can use the Client_t to figure
        // out the id / name
        server_p->send_client_packet_to_all(
            ClientPacket({.client_id = SERVER_CLIENT_ID,
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
            ClientPacket({.client_id = SERVER_CLIENT_ID,
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
    }

    void server_enqueue_message_string(const Client_t& incoming_client,
                                       const std::string& msg) {
        incoming_message_queue.push_back(std::make_pair(incoming_client, msg));
    }

    void server_process_message_string(const ClientMessage& client_message) {
        TRACY_ZONE_SCOPED;
        // Note: not using structured binding since they cannot be captured by
        // lambda expr yet
        const Client_t& incoming_client = client_message.first;
        const std::string& msg = client_message.second;

        const ClientPacket packet =
            network::internal::deserialize_to_packet(msg);

        switch (packet.msg_type) {
            case ClientPacket::MsgType::Announcement: {
                return process_announcement_packet(incoming_client, packet);
            } break;
            case ClientPacket::MsgType::PlayerControl: {
                return process_player_control_packet(incoming_client, packet);
            } break;
            case ClientPacket::MsgType::PlayerJoin: {
                return process_player_join_packet(incoming_client, packet);
            } break;
            case ClientPacket::MsgType::PlayerLeave: {
                return process_player_leave_packet(incoming_client, packet);
            } break;
            default:
                // No clue so lets just send it to everyone except the guy that
                // sent it to us
                server_p->send_client_packet_to_all(
                    packet, [&](Client_t& client) {
                        return client.client_id == incoming_client.client_id;
                    });
                log_warn("Server: {} not handled yet ", packet.msg_type);
                break;
        }
    }
};

}  // namespace network
