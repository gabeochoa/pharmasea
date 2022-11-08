

#pragma once

#include "../globals.h"
//
#include "../entities.h"
#include "../settings.h"
//
#include "shared.h"
//
#include "client.h"
#include "server.h"

namespace network {

static void log(std::string msg) { std::cout << msg << std::endl; }
// std::this_thread::sleep_for(std::chrono::milliseconds(500));

static SteamNetworkingMicroseconds START_TIME;

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
    enum Role {
        s_None = 1 << 0,
        s_Host = 1 << 1,
        s_Client = 1 << 2,
    } desired_role = s_None;

    bool is_host() { return desired_role & s_Host; }
    bool is_client() { return desired_role & s_Client; }
    bool has_role() { return is_host() || is_client(); }

    void set_role(Role role) {
        switch (role) {
            case Role::s_Host: {
                desired_role = Role::s_Host;
                server.reset(new Server(DEFAULT_PORT));
                //
                client.reset(new Client());
                client->update_username(Settings::get().data.username);
                client->lock_in_ip();

            } break;
            case Role::s_Client: {
                desired_role = Role::s_Client;
                client.reset(new Client());
                client->update_username(Settings::get().data.username);
            } break;
            default:
                break;
        }
    }

    std::shared_ptr<Server> server;
    std::shared_ptr<Client> client;
    bool username_set = false;

    void lock_in_username() { username_set = true; }
    void unlock_username() { username_set = false; }
    std::string& host_ip_address() { return client->conn_info.host_ip_address; }
    void lock_in_ip() { client->lock_in_ip(); }
    bool has_set_ip() { return client->conn_info.ip_set; }
    void start_game() { server->send_menu_state(Menu::State::Game); }

    Info() {}

    ~Info() {
        server.reset();
        client.reset();
        //
        desired_role = Role::s_None;
    }

    static void init_connections() {
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

    static void shutdown_connections() {
#ifdef BUILD_WITHOUT_STEAM
        GameNetworkingSockets_Kill();
#endif
    }

    void tick(float dt) {
        if (is_host()) {
            server->tick(dt);
            client->tick(dt);
        }

        if (is_client()) {
            client->tick(dt);
        }
    }
};

}  // namespace network
