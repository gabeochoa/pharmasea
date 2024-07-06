
#include "client_server_comm.h"

#include "network/server.h"

namespace server_only {
void play_sound(const vec2& location, const std::string& sound_name) {
    network::Server::play_sound(location, sound_name);
}

void set_show_minimap() {
    if (!is_server()) {
        log_warn(
            "you are calling a server only function from a client "
            "context, this is probably gonna crash");
    }
    network::Server* server = GLOBALS.get_ptr<network::Server>("server");
    server->get_map_SERVER_ONLY()->showMinimap = true;
}
void set_hide_minimap() {
    if (!is_server()) {
        log_warn(
            "you are calling a server only function from a client "
            "context, this is probably gonna crash");
    }
    network::Server* server = GLOBALS.get_ptr<network::Server>("server");
    server->get_map_SERVER_ONLY()->showMinimap = false;
}

void update_seed(const std::string& seed) {
    if (!is_server()) {
        log_warn(
            "you are calling a server only function from a client "
            "context, this is probably gonna crash");
    }
    network::Server* server = GLOBALS.get_ptr<network::Server>("server");
    server->get_map_SERVER_ONLY()->update_seed(seed);
}
std::string get_current_seed() {
    if (!is_server()) {
        log_warn(
            "you are calling a server only function from a client "
            "context, this is probably gonna crash");
    }
    network::Server* server = GLOBALS.get_ptr<network::Server>("server");
    return server->get_map_SERVER_ONLY()->seed;
}
}  // namespace server_only
