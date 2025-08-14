

#pragma once

#include <thread>
//
#include "../engine/atomic_queue.h"
#include "../engine/tracy.h"
#include "../engine/trigger_on_dt.h"
#include "shared.h"
//
#include "internal/server.h"
//
#include "../map.h"
#include "steam/steamnetworkingtypes.h"

namespace network {

using ClientMessage = std::pair<internal::Client_t, std::string>;

struct Server {
    static std::thread start(int port);
    static void queue_packet(const ClientPacket& p);
    static void forward_packet(const ClientPacket& p);
    static void stop();

    static void play_sound(vec2 position, strings::sounds::SoundId sound);

    //
    void send_player_location_packet(int client_id, const vec3& pos,
                                     float face_direction,
                                     const std::string& name);
    int get_client_id_for_entity(const Entity& entity) {
        for (const auto& player : players) {
            if (player.second->id == entity.id) {
                return player.first;
            }
        }
        return -1;
    }

    std::unique_ptr<Map>& get_map_SERVER_ONLY() { return pharmacy_map; }

   private:
    AtomicQueue<ClientMessage> incoming_message_queue;
    AtomicQueue<ClientPacket> incoming_packet_queue;
    AtomicQueue<ClientPacket> packet_queue;
    std::unique_ptr<internal::Server> server_p;
    std::map<int, std::shared_ptr<Entity>> players;
    std::unique_ptr<Map> pharmacy_map;
    std::atomic<bool> running;
    std::thread::id thread_id;
    std::thread pathfinding_thread;

#if MEASURE_SERVER_PERF
    void fps(float);
    std::array<size_t, 10000> last_frames;
    size_t last_frames_index = 0;
    bool has_looped = false;
#endif

    float next_map_tick_reset = 1.f / 30;  // 60fps
    float next_map_tick = 0;

    float next_player_rare_tick_reset = 1.f / 10;  // 10fps
    float next_player_rare_tick = 0;

    explicit Server(int port) {
        server_p.reset(new internal::Server(port));
        server_p->set_process_message(
            [this](const internal::Client_t& incoming_client,
                   const std::string& msg) {
                this->server_enqueue_message_string(incoming_client, msg);
            });
        server_p->onClientDisconnect =
            ([this](int client_id) { this->process_player_leave(client_id); });

        server_p->onSendClientAnnouncement =
            [this](HSteamNetConnection conn, const std::string& msg,
                   internal::InternalServerAnnouncement type) {
                this->send_announcement(conn, msg, type);
            };
        server_p->startup();

        // TODO add some kind of seed
        // selection screen
        pharmacy_map = std::make_unique<Map>("default_seed");
        GLOBALS.set("server_map", pharmacy_map.get());
        GLOBALS.set("server_players", &players);
        GLOBALS.set("server", this);
    }

    void send_map_state();
    void send_player_rare_data();
    void send_game_state_update();
    void run();
    void tick(float dt);
    void process_incoming_messages();
    void process_incoming_packets();
    void process_packet_forwarding();
    void process_map_update(float dt);
    void process_map_sync(float dt);
    void process_player_rare_tick(float dt);

    void process_announcement_packet(const internal::Client_t&,
                                     const ClientPacket& packet);
    void process_player_control_packet(
        const internal::Client_t& incoming_client, const ClientPacket& packet);

    void send_announcement(HSteamNetConnection conn, const std::string& msg,
                           internal::InternalServerAnnouncement type);

    void process_player_leave(int client_id);

    void process_player_leave_packet(const internal::Client_t& incoming_client,
                                     const ClientPacket&);

    void process_player_join_packet(const internal::Client_t& incoming_client,
                                    const ClientPacket& orig_packet);
    void process_ping_message(const internal::Client_t& incoming_client,
                              const ClientPacket& orig_packet);
    void process_map_seed_info(const internal::Client_t& incoming_client,
                               const ClientPacket& orig_packet);

    void server_enqueue_message_string(
        const internal::Client_t& incoming_client, const std::string& msg);

    void server_process_message_string(const ClientMessage& client_message);
    void server_process_packet(const internal::Client_t&, const ClientPacket&);
    void send_client_packet_to_client(HSteamNetConnection conn,
                                      const ClientPacket& packet);

    void send_client_packet_to_all(
        const ClientPacket& packet,
        const std::function<bool(internal::Client_t&)>& exclude = nullptr);
};

}  // namespace network
