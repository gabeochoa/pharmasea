

#pragma once

#include "../globals.h"
//
#include "../engine/settings.h"
//
#include "../engine/network/webrequest.h"
#include "../engine/statemanager.h"
#include "../engine/trigger_on_dt.h"
#include "shared.h"
//
#include "client.h"
#include "server.h"

namespace network {
struct Info;
}  // namespace network
extern std::unique_ptr<network::Info> network_info;

namespace network {

static SteamNetworkingMicroseconds START_TIME;
static std::string my_remote_ip_address;

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

struct RoleInfoMixin {
    enum Role {
        s_None = 1 << 0,
        s_Host = 1 << 1,
        s_Client = 1 << 2,
    };

    Role desired_role = Role::s_None;
    std::unique_ptr<Client> client;
    std::thread::id client_thread_id;
    std::thread::id server_thread_id;
    std::thread server_thread;

    [[nodiscard]] bool is_host() { return desired_role & s_Host; }
    [[nodiscard]] bool is_client() { return desired_role & s_Client; }
    [[nodiscard]] bool has_role() { return is_host() || is_client(); }
    [[nodiscard]] bool missing_role() { return !has_role(); }

    void set_role(Role role) {
        const auto _setup_client = [&]() {
            client = std::make_unique<Client>();
            client->update_username(Settings::get().data.username);
        };

        switch (role) {
            case Role::s_Host: {
                log_info("set user's role to host");
                desired_role = Role::s_Host;
                server_thread = Server::start(DEFAULT_PORT);
                //
                _setup_client();
                client->lock_in_ip();

            } break;
            case Role::s_Client: {
                log_info("set user's role to client");
                desired_role = Role::s_Client;
                _setup_client();
            } break;
            default:
                break;
        }

        server_thread_id = server_thread.get_id();
        GLOBALS.set("server_thread_id", &server_thread_id);

        client_thread_id = std::this_thread::get_id();
        GLOBALS.set("client_thread_id", &client_thread_id);
    }
};

struct UsernameInfoMixin {
    bool username_set = false;
    void lock_in_username() { username_set = true; }
    void unlock_username() { username_set = false; }
    [[nodiscard]] bool has_username() { return username_set; }
    [[nodiscard]] bool missing_username() { return !has_username(); }
};

struct Info : public RoleInfoMixin, UsernameInfoMixin {
    std::string& host_ip_address() { return client->conn_info.host_ip_address; }
    void lock_in_ip() { client->lock_in_ip(); }
    [[nodiscard]] bool has_set_ip() { return client->conn_info.ip_set; }
    [[nodiscard]] bool has_not_set_ip() { return !has_set_ip(); }

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
            // TODO return true / false so we can hide host / join button?
            // TODO display message to the user?
        }
#endif
        START_TIME = SteamNetworkingUtils()->GetLocalTimestamp();
        SteamNetworkingUtils()->SetDebugOutputFunction(
            k_ESteamNetworkingSocketsDebugOutputType_Msg, log_debug);

        reset_connections();
    }

    static void reset_connections() {
        network_info = std::make_unique<network::Info>();
        if (network::ENABLE_REMOTE_IP) {
            my_remote_ip_address = get_remote_ip_address().value_or("");
        } else {
            my_remote_ip_address = "(DEV) network disabled";
        }

        MenuState::get().set(menu::State::Root);
        GameState::get().set(game::State::InMenu);
    }

    static void shutdown_connections() {
#ifdef BUILD_WITHOUT_STEAM
        GameNetworkingSockets_Kill();
#endif
    }

    void send_updated_seed(const std::string& seed) {
        if (!is_host()) return;
        client->send_updated_seed(seed);
    }

    void send_current_menu_state() {
        if (is_host()) {
            client->send_current_menu_state();
        }
    }

    void tick(float dt) {
        if (missing_role()) return;
        if (has_not_set_ip()) return;

        client->tick(dt);
    }
};

}  // namespace network
