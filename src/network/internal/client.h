
#pragma once

#ifdef __APPLE__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#pragma clang diagnostic ignored "-Wdeprecated-volatile"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#ifdef WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

#include <steam/isteamnetworkingutils.h>
#include <steam/steamnetworkingsockets.h>
#include <steam/steamnetworkingtypes.h>

#ifdef __APPLE__
#pragma clang diagnostic pop
#else
#pragma enable_warn
#endif

#ifdef WIN32
#pragma GCC diagnostic pop
#endif

#include <functional>
#include <string>

#include "../../engine/log.h"
#include "channel.h"
#include "steam/isteamnetworkingsockets.h"

namespace network {
namespace internal {

struct Client {
    SteamNetworkingIPAddr address;
    // NOTE: will be set in `startup`
    ISteamNetworkingSockets *interface = nullptr;
    HSteamNetConnection connection;
    inline static Client *callback_instance;
    bool running = false;
    std::string username;
    std::function<void(std::string)> process_message_cb;

    Client() {}
    ~Client();

    [[nodiscard]] bool is_connected() const {
        return connection != k_HSteamNetConnection_Invalid;
    }

    [[nodiscard]] bool is_not_connected() const { return !is_connected(); }

    void set_process_message(const std::function<void(std::string)> &cb) {
        process_message_cb = cb;
    }

    void set_address(SteamNetworkingIPAddr addy) { address = addy; }

    void set_address(const std::string &ip);

    void startup();

    bool poll_incoming_messages();

    bool run();

    void send_string_to_server(const std::string &msg, Channel channel) {
        interface->SendMessageToConnection(
            connection, msg.c_str(), (uint32) msg.length(), channel, nullptr);
    }

    void send_join_info_request();
    void send_leave_info_request();
    void on_steam_network_connection_status_changed(
        SteamNetConnectionStatusChangedCallback_t *info);

    static void SteamNetConnectionStatusChangedCallback(
        SteamNetConnectionStatusChangedCallback_t *pInfo) {
        if (!callback_instance) {
            log_warn(
                "client callback instance saw a change but wasnt initialized "
                "still");
            return;
        }
        callback_instance->on_steam_network_connection_status_changed(pInfo);
    }
};

}  // namespace internal
}  // namespace network
