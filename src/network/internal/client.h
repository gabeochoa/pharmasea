
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

struct IClient {
    virtual ~IClient() = default;

    // Provided by caller (outer network::Client)
    std::string username;

    virtual void set_process_message(const std::function<void(std::string)> &cb) = 0;
    virtual void set_address(SteamNetworkingIPAddr addy) = 0;
    virtual void set_address(const std::string &ip) = 0;
    virtual void startup() = 0;
    virtual bool run() = 0;
    virtual void send_string_to_server(const std::string &msg, Channel channel) = 0;
    [[nodiscard]] virtual bool is_connected() const = 0;
    [[nodiscard]] virtual bool is_not_connected() const = 0;

    // Common join/leave helpers (implemented in client.cpp).
    void send_join_info_request();
    void send_leave_info_request();
};

// GameNetworkingSockets implementation (current behavior).
struct GnsClient : public IClient {
    SteamNetworkingIPAddr address;
    // NOTE: will be set in `startup`
    ISteamNetworkingSockets *interface = nullptr;
    HSteamNetConnection connection;
    inline static GnsClient *callback_instance;
    bool running = false;
    std::function<void(std::string)> process_message_cb;

    GnsClient() {}
    ~GnsClient() override;

    [[nodiscard]] bool is_connected() const override {
        return connection != k_HSteamNetConnection_Invalid;
    }

    [[nodiscard]] bool is_not_connected() const override {
        return !is_connected();
    }

    void set_process_message(
        const std::function<void(std::string)> &cb) override {
        process_message_cb = cb;
    }

    void set_address(SteamNetworkingIPAddr addy) override { address = addy; }

    void set_address(const std::string &ip) override;

    void startup() override;

    bool poll_incoming_messages();

    bool run() override;

    void send_string_to_server(const std::string &msg,
                               Channel channel) override {
        interface->SendMessageToConnection(
            connection, msg.c_str(), (uint32) msg.length(), channel, nullptr);
    }

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

// In-process implementation (no sockets).
struct LocalClient : public IClient {
    // NOTE: not a real Steam connection; just an identifier.
    HSteamNetConnection connection = k_HSteamNetConnection_Invalid;
    bool running = false;
    std::function<void(std::string)> process_message_cb;

    LocalClient() {}
    ~LocalClient() override;

    void set_process_message(
        const std::function<void(std::string)> &cb) override {
        process_message_cb = cb;
    }

    // Address is ignored in local-only mode, but keep API compatible.
    void set_address(SteamNetworkingIPAddr) override {}
    void set_address(const std::string &) override {}

    void startup() override;
    bool run() override;

    void send_string_to_server(const std::string &msg, Channel) override;

    [[nodiscard]] bool is_connected() const override {
        return connection != k_HSteamNetConnection_Invalid;
    }
    [[nodiscard]] bool is_not_connected() const override { return !is_connected(); }
};

}  // namespace internal
}  // namespace network
