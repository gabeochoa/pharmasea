

#pragma once

#include <optional>
#include <thread>
#include <unordered_map>
//
#include "../engine/atomic_queue.h"
#include "../engine/runtime_globals.h"
#include "../engine/tracy.h"
#include "../engine/trigger_on_dt.h"
#include "../entity.h"
#include "types.h"
//
#include "internal/server.h"
//
#include "../map.h"
#include "steam/steamnetworkingtypes.h"

namespace network {

using ClientMessage = std::pair<internal::Client_t, std::string>;

struct Server {
    // Starts the server thread (idempotent).
    static void start(int port);
    static void queue_packet(const ClientPacket& p);
    static void forward_packet(const ClientPacket& p);
    // Stops the server thread and waits for it to exit (idempotent).
    static void shutdown();
    ~Server();

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
    void force_send_map_state();

   private:
    AtomicQueue<ClientMessage> incoming_message_queue;
    AtomicQueue<ClientPacket> incoming_packet_queue;
    AtomicQueue<ClientPacket> packet_queue;
    std::unique_ptr<internal::IServer> server_p;
    // Connection <-> client_id mapping owned by authoritative server layer.
    // Transport should not assign ids.
    std::unordered_map<HSteamNetConnection, int> conn_to_client_id;
    std::unordered_map<int, HSteamNetConnection> client_id_to_conn;
    int next_client_id = 10000;
    std::map<int, std::shared_ptr<Entity>> players;
    std::unique_ptr<Map> pharmacy_map;
    std::atomic<bool> running;
    std::thread server_thread;
    std::thread pathfinding_thread;

#if MEASURE_SERVER_PERF
    void fps(float);
    std::array<size_t, 10000> last_frames;
    size_t last_frames_index = 0;
    bool has_looped = false;
#endif

    // Full world snapshot sync. If this is too low it looks "teleporty" on
    // clients; if it's too high it can become expensive.
    // Snapshot sync is currently a full-world blob and can be large.
    // Keep this fairly low-frequency to avoid overwhelming the network stack.
    float next_map_tick_reset = 1.f / 20;  // 20fps
    float next_map_tick = 0;

    float next_player_rare_tick_reset = 1.f / 100;  // 100fps
    float next_player_rare_tick = 0;

    explicit Server(int port) {
        log_info("Server constructor called with port: {}", port);
        if (network::LOCAL_ONLY) {
            log_info("Creating internal::LocalServer instance (local-only)");
            server_p = std::make_unique<internal::LocalServer>();
        } else {
            log_info("Creating internal::GnsServer instance");
            server_p = std::make_unique<internal::GnsServer>(port);
        }
        log_info("Setting up server callbacks");
        server_p->set_process_message(
            [this](HSteamNetConnection conn, const std::string& msg) {
                this->server_enqueue_message_string(conn, msg);
            });
        server_p->set_on_client_disconnect([this](HSteamNetConnection conn) {
            this->process_disconnect(conn);
        });
        log_info("Calling server_p->startup()");
        server_p->startup();
        log_info("server_p->startup() completed, running state: {}",
                 server_p->is_running() ? "true" : "false");

        // TODO add some kind of seed
        // selection screen
        log_info("Creating pharmacy_map with default_seed");
        pharmacy_map = std::make_unique<Map>("default_seed");
        globals::set_server(this);
        log_info("Server constructor completed");
    }

    void send_map_state(Channel channel);
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

    void server_enqueue_message_string(HSteamNetConnection conn,
                                       const std::string& msg);
    void process_disconnect(HSteamNetConnection conn);
    [[nodiscard]] int allocate_client_id();
    [[nodiscard]] std::optional<int> lookup_client_id(
        HSteamNetConnection conn) const;
    [[nodiscard]] std::vector<int> connected_client_ids() const;

    void server_process_message_string(const ClientMessage& client_message);
    void server_process_packet(const internal::Client_t&, const ClientPacket&);
    void send_client_packet_to_client(HSteamNetConnection conn,
                                      const ClientPacket& packet);

    void send_client_packet_to_all(
        const ClientPacket& packet,
        const std::function<bool(internal::Client_t&)>& exclude = nullptr);
};

}  // namespace network
