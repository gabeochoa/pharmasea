

#pragma once

#include "internal/client.h"
//
#include "../engine/globals_register.h"
#include "../engine/log.h"
#include "../engine/sound_library.h"

namespace network {
extern long long total_ping;
extern long long there_ping;
extern long long return_ping;

struct Client {
    // TODO eventually generate room codes instead of IP address
    // can be some hash of the real ip that the we can decode
    struct ConnectionInfo {
        std::string host_ip_address = "127.0.0.1";
        bool ip_set = false;
    } conn_info;

    int id = 0;
    std::shared_ptr<internal::Client> client_p;
    std::map<int, std::shared_ptr<Entity>> remote_players;
    std::shared_ptr<Map> map;
    std::vector<ClientPacket::AnnouncementInfo> announcements;

    float next_tick_reset = 0.04f;
    float next_tick = 0.0f;

    float next_ping_reset = 0.2f;
    float next_ping = 0.0f;

    Client();
    void update_username(const std::string& new_name);
    void lock_in_ip();
    void tick(float dt);
    void send_ping_packet(int my_id, float dt);
    void send_player_input_packet(int my_id);
    void send_current_menu_state();
    void client_process_message_string(const std::string& msg);
};

}  // namespace network
