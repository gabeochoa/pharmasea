

#pragma once

#include "../globals.h"
//
#include "../engine/settings.h"
//
#include "../engine/statemanager.h"
#include "../engine/trigger_on_dt.h"
#include "shared.h"
//
#include "client.h"
#include "server.h"

namespace network {

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

    [[nodiscard]] bool is_host() { return desired_role & s_Host; }
    [[nodiscard]] bool is_client() { return desired_role & s_Client; }
    [[nodiscard]] bool has_role() { return is_host() || is_client(); }
    [[nodiscard]] bool missing_role() { return !has_role(); }

    void set_role(Role role) {
        switch (role) {
            case Role::s_Host: {
                log_info("set user's role to host");
                desired_role = Role::s_Host;
                server_thread = Server::start(DEFAULT_PORT);
                //
                client = std::make_shared<Client>();
                client->update_username(Settings::get().data.username);
                client->lock_in_ip();

            } break;
            case Role::s_Client: {
                log_info("set user's role to client");
                desired_role = Role::s_Client;
                client = std::make_shared<Client>();
                client->update_username(Settings::get().data.username);
            } break;
            default:
                break;
        }

        server_thread_id = server_thread.get_id();
        GLOBALS.set("server_thread_id", &server_thread_id);

        client_thread_id = std::this_thread::get_id();
        GLOBALS.set("client_thread_id", &client_thread_id);
    }

    std::thread::id client_thread_id;
    std::thread::id server_thread_id;
    std::shared_ptr<Client> client;
    std::thread server_thread;

    bool username_set = false;

    void lock_in_username() { username_set = true; }
    void unlock_username() { username_set = false; }
    std::string& host_ip_address() { return client->conn_info.host_ip_address; }
    void lock_in_ip() { client->lock_in_ip(); }
    [[nodiscard]] bool has_set_ip() { return client->conn_info.ip_set; }
    [[nodiscard]] bool has_not_set_ip() { return !has_set_ip(); }
    [[nodiscard]] bool has_username() { return username_set; }
    [[nodiscard]] bool missing_username() { return !has_username(); }

    TriggerOnDt menu_state_tick_trigger = TriggerOnDt(1.0f);

    Info() {}

    ~Info() {
        desired_role = Role::s_None;

        // cleanup server
        {
            if (server_thread_id == std::thread::id()) return;
            Server::stop();
            server_thread.join();
        }
    }

    static void init_connections() {
#ifdef BUILD_WITHOUT_STEAM
        SteamDatagramErrMsg errMsg;
        if (!GameNetworkingSockets_Init(nullptr, errMsg)) {
            log_warn("GameNetworkingSockets init failed {}", errMsg);
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

    // TODO should probably move to network/client.
    // Right now we only have the is_host() check here
    void send_current_menu_state(float dt) {
        bool run = menu_state_tick_trigger.test(dt);
        if (!run) return;

        ClientPacket packet({
            .client_id = SERVER_CLIENT_ID,
            .msg_type = ClientPacket::MsgType::GameState,
            .msg = ClientPacket::GameStateInfo({
                .host_menu_state = MenuState::get().read(),
                .host_game_state = GameState::get().read(),
            }),
        });
        Server::forward_packet(packet);
    }

    void send_updated_seed(const std::string& seed) {
        if (!is_host()) return;

        ClientPacket packet{
            .client_id = SERVER_CLIENT_ID,
            .msg_type = ClientPacket::MsgType::MapSeed,
            .msg = ClientPacket::MapSeedInfo{.seed = seed},
        };

        Server::queue_packet(packet);
    }

    void tick(float dt) {
        if (missing_role()) return;
        if (has_not_set_ip()) return;

        if (is_host()) {
            send_current_menu_state(dt);
        }

        client->tick(dt);
    }
};

}  // namespace network
