

#pragma once

#include "../globals.h"
//
#include "../engine/settings.h"
//
#include "../engine/network/webrequest.h"
#include "../engine/statemanager.h"
#include "../engine/trigger_on_dt.h"
#include "types.h"
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

    [[nodiscard]] bool is_host() { return desired_role & s_Host; }
    [[nodiscard]] bool is_client() { return desired_role & s_Client; }
    [[nodiscard]] bool has_role() { return is_host() || is_client(); }
    [[nodiscard]] bool missing_role() { return !has_role(); }

    void set_role(Role role) {
        log_info("set_role called with role: {}", (int) role);
        const auto _setup_client = [&]() {
            log_info("_setup_client lambda called - creating Client instance");
            client = std::make_unique<Client>();
            client->update_username(Settings::get().data.username);
            log_info("_setup_client lambda completed");
        };

        // Local-only mode does not support joining by IP; treat "Client" as
        // "Host local" so the UI remains usable.
        if (network::LOCAL_ONLY && role == Role::s_Client) {
            role = Role::s_Host;
        }

        switch (role) {
            case Role::s_Host: {
                log_info("set user's role to host");
                desired_role = Role::s_Host;
                log_info("Calling Server::start with port: {}", DEFAULT_PORT);
                Server::start(DEFAULT_PORT);
                log_info("Server::start returned");
                //
                _setup_client();
                client->lock_in_ip();
                log_info("Host role setup completed");

            } break;
            case Role::s_Client: {
                log_info("set user's role to client");
                desired_role = Role::s_Client;
                _setup_client();
            } break;
            default:
                log_warn("set_role called with unknown role: {}", (int) role);
                break;
        }

        server_thread_id = Server::get_thread_id();
        GLOBALS.set("server_thread_id", &server_thread_id);
        log_info("Server thread ID set: {}", (void*) &server_thread_id);

        client_thread_id = std::this_thread::get_id();
        GLOBALS.set("client_thread_id", &client_thread_id);
        log_info("set_role completed successfully");
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
        log_info("network::Info destructor called");
        desired_role = Role::s_None;
        // cleanup server
        {
            if (server_thread_id == std::thread::id()) {
                log_info("No server thread to clean up (thread_id is default)");
                return;
            }
            log_info("Stopping server and joining thread");
            Server::shutdown();
        }
        log_info("network::Info destructor completed");
    }

    static void init_connections() {
        log_info("init_connections() called");
#ifdef BUILD_WITHOUT_STEAM
        log_info(
            "BUILD_WITHOUT_STEAM is defined, calling "
            "GameNetworkingSockets_Init()");
        SteamDatagramErrMsg errMsg;
        bool init_result = GameNetworkingSockets_Init(nullptr, errMsg);
        log_info("GameNetworkingSockets_Init() returned: {}",
                 init_result ? "true" : "false");
        if (!init_result) {
            log_warn("GameNetworkingSockets init failed: {}", errMsg);
            // TODO return true / false so we can hide host / join button?
            // TODO display message to the user?
        } else {
            log_info("GameNetworkingSockets_Init() succeeded");
        }
#else
        log_info(
            "BUILD_WITHOUT_STEAM is NOT defined, skipping "
            "GameNetworkingSockets_Init()");
#endif
        log_info("Calling SteamNetworkingUtils()->GetLocalTimestamp()");
        START_TIME = SteamNetworkingUtils()->GetLocalTimestamp();
        log_info("Setting debug output function");
        SteamNetworkingUtils()->SetDebugOutputFunction(
            k_ESteamNetworkingSocketsDebugOutputType_Msg, log_debug);

        reset_connections();
        log_info("Initializing GNS Network Connections");
    }

    static void reset_connections() {
        network_info = std::make_unique<network::Info>();
        if (network::LOCAL_ONLY) {
            my_remote_ip_address = "Local only";
        } else if (network::ENABLE_REMOTE_IP) {
            my_remote_ip_address = get_remote_ip_address().value_or("");
        } else {
            my_remote_ip_address = "(DEV) network disabled";
        }

        // In local-only mode, boot straight into the Network lobby flow.
        // If username is missing, NetworkLayer will prompt for it.
        if (network::LOCAL_ONLY) {
            MenuState::get().set(menu::State::Network);
            GameState::get().set(game::State::InMenu);

            if (!Settings::get().data.username.empty()) {
                network_info->lock_in_username();
                network_info->set_role(Role::s_Host);
            }
            return;
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
