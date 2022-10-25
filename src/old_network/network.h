
#pragma once


#include <vector>
#include <functional>
#include <string>
//
#include "shared.h"

namespace enetpp {
class client;

template<typename T>
class server;
}  // namespace enetpp

namespace network {

using Buffer = std::vector<unsigned char>;

struct ThinClient {
    int _uid;
    int get_id() const { return _uid; }
};

struct Info {
    enum State {
        s_None = 1 << 0,
        s_Host = 1 << 1,
        s_Client = 1 << 2,
    } desired_role = s_None;

    enetpp::server<ThinClient>* server;
    enetpp::client* client;

    int my_client_id = -1;
    // std::wstring username = L"葛城 ミサト";
    std::string username = "";
    bool username_set = false;

    std::function<network::ClientPacket(int)> player_packet_info_cb;
    std::function<void(int, int)> add_new_player_cb;
    std::function<void(int)> remove_player_cb;
    std::function<void(int, std::string, float[3], int)>
        update_remote_player_cb;

    Info();
    ~Info();

    void register_new_player_cb(std::function<void(int, int)> cb) {
        add_new_player_cb = cb;
    }

    void register_remove_player_cb(std::function<void(int)> cb) {
        remove_player_cb = cb;
    }

    void register_update_player_cb(
        std::function<void(int, std::string, float[3], int)> cb) {
        update_remote_player_cb = cb;
    }

    void register_player_packet_cb(
        std::function<network::ClientPacket(int)> cb) {
        player_packet_info_cb = cb;
    }

    void close_active_roles();

    void set_role_to_client() {
        close_active_roles();
        this->desired_role = Info::State::s_Client;
    }

    void set_role_to_none() {
        close_active_roles();
        this->desired_role = Info::State::s_None;
    }

    void set_role_to_host() {
        close_active_roles();
        this->desired_role = Info::State::s_Host;
    }

    bool is_host() { return this->desired_role & Info::State::s_Host; }
    bool is_client() { return this->desired_role & Info::State::s_Client; }
    bool has_role() { return is_host() || is_client(); }
    void fwd_packet_to_other_clients(int source_id, ClientPacket packet);
    void fwd_packet_to_source(int source_id, ClientPacket packet);
    void process_client_packet_msg(int client_id, ClientPacket packet);
    void start_server();
    void start_client();
    void client_send_player();
    void host_send_player();
    void network_tick(float dt);
    void send_updated_state();
    Buffer get_player_packet();
};

}  // namespace network
   //
