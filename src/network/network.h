

#pragma once

#include "shared.h"
//
#include "client.h"
#include "server.h"

namespace network {

void log(std::string msg) { std::cout << msg << std::endl; }

SteamNetworkingMicroseconds START_TIME;

static void log_debug(ESteamNetworkingSocketsDebugOutputType eType,
                      const char* pszMsg) {
    SteamNetworkingMicroseconds time =
        SteamNetworkingUtils()->GetLocalTimestamp() - START_TIME;
    printf("%10.6f %s\n", time * 1e-6, pszMsg);
    fflush(stdout);
    if (eType == k_ESteamNetworkingSocketsDebugOutputType_Bug) {
        fflush(stdout);
        fflush(stderr);
        exit(1);
    }
}

struct Info {
    int my_client_id;
    std::string username = "default username";
    bool username_set = true;
    std::string host_ip_address;
    bool ip_set = false;

    enum State {
        s_None = 1 << 0,
        s_Host = 1 << 1,
        s_Client = 1 << 2,
    } desired_role = s_None;

    std::shared_ptr<Server> server;
    std::shared_ptr<Client> client;

    std::function<network::ClientPacket(int)> player_packet_info_cb;
    std::function<void(int, int)> add_new_player_cb;
    std::function<void(int)> remove_player_cb;
    std::function<void(int, std::string, float[3], int)>
        update_remote_player_cb;

    bool is_host() { return desired_role & s_Host; }
    bool is_client() { return desired_role & s_Client; }
    bool has_role() { return is_host() || is_client(); }
    bool has_set_ip() { return is_host() || (is_client() && ip_set); }
    void lock_in_ip() {
        ip_set = true;
        client->set_address(host_ip_address);
        client->startup();
    }

    void init_connections() {
#ifdef BUILD_WITHOUT_STEAM
        SteamDatagramErrMsg errMsg;
        if (!GameNetworkingSockets_Init(nullptr, errMsg)) {
            log(fmt::format("GameNetworkingSockets init failed {}", errMsg));
        }
#endif
        START_TIME = SteamNetworkingUtils()->GetLocalTimestamp();
        SteamNetworkingUtils()->SetDebugOutputFunction(
            k_ESteamNetworkingSocketsDebugOutputType_Msg, log_debug);
    }

    void shutdown_connections() {
#ifdef BUILD_WITHOUT_STEAM
        GameNetworkingSockets_Kill();
#endif
    }

    void set_role_to_host() {
        desired_role = s_Host;
        init_connections();
        server.reset(new Server(DEFAULT_PORT));
        server->startup();
    }

    void set_role_to_client() {
        desired_role = s_Client;
        init_connections();
        client.reset(new Client());
    }

    void set_role_to_none() {
        desired_role = s_None;
        ip_set = false;
        server->teardown();
        server.reset();
        client.reset();
        shutdown_connections();
    }

    void tick(float) {
        if (desired_role & s_Host) {
            server->run();
            return;
        }

        if (desired_role & s_Client) {
            client->run();
            return;
        }
    }

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

    int run() {
        desired_role = s_Host;
        // desired_role = s_Client;

        // if (desired_role & s_Host) {
        // log("I'm the host");
        // Server server(770);
        // server.startup();
        // while (server.run()) {
        // std::this_thread::sleep_for(std::chrono::milliseconds(10));
        // };
        // }
        //
        // if (desired_role & s_Client) {
        // log("im the client");
        // SteamNetworkingIPAddr address;
        // address.ParseString("127.0.0.1");
        // address.m_port = 770;
        // Client client(address);
        // client.startup();
        // while (client.run()) {
        // std::this_thread::sleep_for(std::chrono::milliseconds(10));
        // }
        // }

        shutdown_connections();
        return 0;
    }

    void send_updated_state() {
        ClientPacket player({
            .client_id = my_client_id,
            .msg_type = ClientPacket::MsgType::GameState,
            .msg = ClientPacket::GameStateInfo(
                {.host_menu_state = Menu::get().state}),
        });

        Buffer buffer;
        bitsery::quickSerialization(OutputAdapter{buffer}, player);
        server->send_string_to_all(buffer);
    }
};

}  // namespace network
