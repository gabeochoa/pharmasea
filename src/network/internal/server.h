
#pragma once

#include <steam/isteamnetworkingutils.h>
#include <steam/steamnetworkingsockets.h>
#include <steam/steamnetworkingtypes.h>

#include <chrono>
#include <thread>
#include <vector>

#include "../../engine/assert.h"
#include "../../engine/toastmanager.h"
#include "channel.h"
#include "steam/isteamnetworkingsockets.h"

#include "local_hub.h"

namespace network {
namespace internal {

struct Client_t {
    HSteamNetConnection conn = k_HSteamNetConnection_Invalid;
    int client_id = -1;  // Assigned by authoritative network::Server
};

enum struct InternalServerAnnouncement {
    Info,
    Warn,
    Error,
};

struct Server {
    SteamNetworkingIPAddr address;
    // Note: initialized in `startup()`
    ISteamNetworkingSockets *interface = nullptr;
    HSteamListenSocket listen_sock;
    HSteamNetPollGroup poll_group;
    inline static Server *callback_instance;
    bool running = false;
    std::function<void(HSteamNetConnection, std::string)> process_message_cb;
    // TODO add setter and make private
    std::function<void(HSteamNetConnection)> onClientDisconnect;
    std::function<void(HSteamNetConnection, std::string,
                       InternalServerAnnouncement)>
        onSendClientAnnouncement;

    void set_process_message(
        const std::function<void(HSteamNetConnection, std::string)> &cb) {
        process_message_cb = cb;
    }

    void set_announcement_cb(
        const std::function<void(HSteamNetConnection, std::string,
                                 InternalServerAnnouncement)> &cb) {
        onSendClientAnnouncement = cb;
    }

    // Track active connections (client_id is assigned above transport layer).
    std::map<HSteamNetConnection, Client_t> clients;

    explicit Server(int port) {
        VALIDATE(port > 0 && port < 65535, "invalid port");
        address.Clear();
        address.m_port = (unsigned short) port;
        callback_instance = this;
    }

    ~Server() {
        log_info(
            "internal::Server destructor called, running: {}, interface: {}",
            running ? "true" : "false", (void *) interface);
        running = false;
        if (!interface) {
            log_info("No interface to clean up, clearing clients");
            clients.clear();
            return;
        }

        log_info("Cleaning up {} client connections (no announcements)",
                 clients.size());
        for (auto it : clients) {
            interface->CloseConnection(it.first, 0, "server shutdown", true);
        }
        clients.clear();
        log_info("Closing listen socket");
        interface->CloseListenSocket(listen_sock);
        listen_sock = k_HSteamListenSocket_Invalid;
        log_info("Destroying poll group");
        interface->DestroyPollGroup(poll_group);
        poll_group = k_HSteamNetPollGroup_Invalid;
        log_info("internal::Server destructor cleanup completed");
    }

    void send_announcement_to_client(HSteamNetConnection conn,
                                     const std::string &msg,
                                     InternalServerAnnouncement type) {
        if (onSendClientAnnouncement) onSendClientAnnouncement(conn, msg, type);
    }

    void send_announcement_to_all(
        const std::string &msg, InternalServerAnnouncement type,
        const std::function<bool(Client_t &)> &exclude = nullptr) {
        if (onSendClientAnnouncement) {
            for (auto &c : clients) {
                if (exclude && exclude(c.second)) continue;
                onSendClientAnnouncement(c.first, msg, type);
            }
        }
    }

    void send_message_to_all(
        const char *buffer, uint32 size,
        const std::function<bool(Client_t &)> &exclude = nullptr) {
        for (auto &c : clients) {
            if (exclude && exclude(c.second)) continue;
            send_message_to_connection(c.first, buffer, size);
        }
    }

    void send_message_to_connection(HSteamNetConnection conn,
                                    const char *buffer, uint32 size) {
        interface->SendMessageToConnection(conn, buffer, size,
                                           Channel::RELIABLE, nullptr);
        // TODO why does unreliable make it so unreliable...
        // eventually we want the packet to figure out if it should matter or
        // not
        // packet.channel, nullptr);
        //
        // TODO whats going on here is that the player join packet is not making
        // it and doesnt get resend on unreliable, which causes crashes we
        // should tell it which ones can be reliable and which we are okay with
        // not
    }

    bool run() {
        TRACY_ZONE_SCOPED;
        if (!running) return false;

        auto poll_incoming_messages = [&]() {
            ISteamNetworkingMessage *incoming_msg = nullptr;
            // TODO Should we run this for more messages?
            int num_msgs = interface->ReceiveMessagesOnPollGroup(
                poll_group, &incoming_msg, 1 /* num max messages*/);
            if (num_msgs == 0) return;
            if (num_msgs == -1) {
                VALIDATE(false, "Failed checking for messages");
                return;
            }

            std::string cmd;
            cmd.assign((const char *) incoming_msg->m_pData,
                       incoming_msg->m_cbSize);
            HSteamNetConnection conn = incoming_msg->m_conn;
            incoming_msg->Release();

            //
            if (this->process_message_cb) this->process_message_cb(conn, cmd);
        };
        auto poll_connection_state_changes = [&]() {
            Server::callback_instance = this;
            interface->RunCallbacks();
        };
        poll_incoming_messages();
        poll_connection_state_changes();
        return true;
    }

    void startup() {
        log_info("internal::Server::startup() called");
        log_info("Calling SteamNetworkingSockets() to get interface");
        interface = SteamNetworkingSockets();
        log_info("SteamNetworkingSockets() returned: {}", (void *) interface);

        if (interface == nullptr) {
            log_warn(
                "Failed to initialize GameNetworkingSockets (SNS). Hosting "
                "disabled");
            log_warn(
                "SteamNetworkingSockets() returned nullptr - this may indicate "
                "GNS was not properly initialized");
            running = false;
            return;
        }
        log_info("GameNetworkingSockets interface obtained successfully");

        /// [connection int32] Upper limit of buffered pending bytes to be sent,
        /// if this is reached SendMessage will return k_EResultLimitExceeded
        /// Default is 512k (524288 bytes)
        SteamNetworkingUtils()->SetGlobalConfigValueInt32(
            k_ESteamNetworkingConfig_SendBufferSize, 1024 * 1024);

        /// [connection int32] Timeout value (in ms) to use after connection is
        /// established
        SteamNetworkingUtils()->SetGlobalConfigValueInt32(
            k_ESteamNetworkingConfig_TimeoutConnected, 10);

        /// [connection int32] Minimum/maximum send rate clamp, in bytes/sec.
        /// At the time of this writing these two options should always be set
        /// to the same value, to manually configure a specific send rate.  The
        /// default value is 256K.  Eventually we hope to have the library
        /// estimate the bandwidth of the channel and set the send rate to that
        /// estimated bandwidth, and these values will only set limits on that
        /// send rate.
        constexpr int rate = 1024 * 1024 * 1024;
        SteamNetworkingUtils()->SetGlobalConfigValueInt32(
            k_ESteamNetworkingConfig_SendRateMin, rate);
        SteamNetworkingUtils()->SetGlobalConfigValueInt32(
            k_ESteamNetworkingConfig_SendRateMax, rate);

        SteamNetworkingConfigValue_t opt;
        opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged,
                   (void *) Server::SteamNetConnectionStatusChangedCallback);

        log_info("Creating listen socket on port {}", address.m_port);
        listen_sock = interface->CreateListenSocketIP(address, 1, &opt);
        if (listen_sock == k_HSteamListenSocket_Invalid) {
            log_warn("Failed to listen on port {}", address.m_port);
            running = false;
            return;
        }
        log_info("Listen socket created successfully: {}",
                 (unsigned long long) listen_sock);
        log_info("Creating poll group");
        poll_group = interface->CreatePollGroup();
        if (poll_group == k_HSteamNetPollGroup_Invalid) {
            log_warn("(poll group) Failed to create poll group on port {}",
                     address.m_port);
            running = false;
            return;
        }
        log_info("Poll group created successfully: {}",
                 (unsigned long long) poll_group);
        log_info("Server listening on port {}", address.m_port);

        running = true;
    }

   private:
    void process_connection_ended(
        SteamNetConnectionStatusChangedCallback_t *info) {
        // Ignore if they were not previously connected.  (If they
        // disconnected before we accepted the connection.)
        if (info->m_eOldState == k_ESteamNetworkingConnectionState_Connected) {
            auto itClient = clients.find(info->m_hConn);
            if (itClient != clients.end()) {
                clients.erase(itClient);
            }
            if (onClientDisconnect) onClientDisconnect(info->m_hConn);

        } else {
            VALIDATE(info->m_eOldState ==
                         k_ESteamNetworkingConnectionState_Connecting,
                     "We expected them not to be connected but they were");
        }

        // Clean up the connection.  This is important!
        // The connection is "closed" in the network sense, but
        // it has not been destroyed.  We must close it on our end, too
        // to finish up.  The reason information do not matter in this
        // case, and we cannot linger because it's already closed on the
        // other end, so we just pass 0's.
        interface->CloseConnection(info->m_hConn, 0, nullptr, false);
    }

    void process_connection_starting(
        SteamNetConnectionStatusChangedCallback_t *info) {
        log_info("Connection request from {}",
                 info->m_info.m_szConnectionDescription);

        // This should be a new connection
        const auto preexisting_client = clients.find(info->m_hConn);
        VALIDATE(preexisting_client == clients.end(),
                 "Client already connected but shouldnt be");

        // A client is attempting to connect
        // Try to accept the connection.
        if (interface->AcceptConnection(info->m_hConn) != k_EResultOK) {
            // This could fail.  If the remote host tried to connect,
            // but then disconnected, the connection may already be half
            // closed.  Just destroy whatever we have on our side.
            interface->CloseConnection(info->m_hConn, 0, nullptr, false);
            log_info("Can't accept connection. (It was already closed?)");
            return;
        }

        // Assign the poll group
        if (!interface->SetConnectionPollGroup(info->m_hConn, poll_group)) {
            interface->CloseConnection(info->m_hConn, 0, nullptr, false);
            log_info("Failed to set poll group?");
            return;
        }

        clients[info->m_hConn] = Client_t{.conn = info->m_hConn, .client_id = -1};
        interface->SetConnectionName(info->m_hConn, "unassigned");
    }

    void connection_changed_callback(
        SteamNetConnectionStatusChangedCallback_t *info) {
        log_info("connection_changed_callback {}", info->m_info.m_eState);

        // What's the state of the connection?
        switch (info->m_info.m_eState) {
            case k_ESteamNetworkingConnectionState_ClosedByPeer:
            case k_ESteamNetworkingConnectionState_ProblemDetectedLocally: {
                process_connection_ended(info);
                break;
            }

            case k_ESteamNetworkingConnectionState_Connecting: {
                process_connection_starting(info);
                break;
            }

            case k_ESteamNetworkingConnectionState_Connected:
                // We will get a callback immediately after accepting the
                // connection. Since we are the server, we can ignore this, it's
                // not news to us.
                break;

            case k_ESteamNetworkingConnectionState_None:
                // NOTE: We will get callbacks here when we destroy connections.
                // You can ignore these.
                break;

            default:
                // Silences -Wswitch
                break;
        }
    }

    static void SteamNetConnectionStatusChangedCallback(
        SteamNetConnectionStatusChangedCallback_t *pInfo) {
        callback_instance->connection_changed_callback(pInfo);
    }
};

// Transport interface for internal server implementations.
struct IServer {
    virtual ~IServer() = default;

    [[nodiscard]] virtual bool is_running() const = 0;

    virtual void set_process_message(
        const std::function<void(HSteamNetConnection, std::string)> &cb) = 0;
    virtual void set_on_client_disconnect(
        const std::function<void(HSteamNetConnection)> &cb) = 0;

    virtual void startup() = 0;
    virtual bool run() = 0;

    virtual void send_message_to_connection(HSteamNetConnection conn,
                                            const char *buffer,
                                            uint32 size,
                                            Channel channel) = 0;
};

// GameNetworkingSockets implementation (existing behavior).
struct GnsServer : public Server, public IServer {
    using Server::Server;

    [[nodiscard]] bool is_running() const override { return Server::running; }

    void set_process_message(
        const std::function<void(HSteamNetConnection, std::string)> &cb) override {
        Server::set_process_message(cb);
    }
    void set_on_client_disconnect(
        const std::function<void(HSteamNetConnection)> &cb) override {
        this->onClientDisconnect = cb;
    }

    void startup() override { Server::startup(); }
    bool run() override { return Server::run(); }

    void send_message_to_connection(HSteamNetConnection conn, const char *buffer,
                                    uint32 size,
                                    Channel channel) override {
        this->interface->SendMessageToConnection(conn, buffer, size, channel,
                                                 nullptr);
    }
};

// In-process implementation (no sockets).
struct LocalServer : public IServer {
    bool running = false;

    [[nodiscard]] bool is_running() const override { return running; }

    void set_process_message(
        const std::function<void(HSteamNetConnection, std::string)> &cb) override {
        process_message_cb = cb;
    }
    void set_on_client_disconnect(
        const std::function<void(HSteamNetConnection)> &cb) override {
        onClientDisconnect = cb;
    }

    void startup() override {
        // Reset and start the hub for this server lifetime.
        local::reset();
        {
            std::lock_guard<std::mutex> lock(local::hub().m);
            local::hub().server_running = true;
        }
        running = true;
    }

    bool run() override {
        if (!running) return false;

        // Process disconnect events.
        for (int i = 0; i < 64; ++i) {
            auto maybe_conn = local::pop_disconnect_event();
            if (!maybe_conn.has_value()) break;
            HSteamNetConnection conn = *maybe_conn;
            if (onClientDisconnect) onClientDisconnect(conn);
        }

        // Drain some client->server messages.
        for (int i = 0; i < 128; ++i) {
            auto maybe = local::pop_to_server();
            if (!maybe.has_value()) break;
            HSteamNetConnection conn = maybe->first;
            std::string &msg = maybe->second;
            if (process_message_cb) process_message_cb(conn, msg);
        }

        return true;
    }

    void send_message_to_connection(HSteamNetConnection conn, const char *buffer,
                                    uint32 size,
                                    Channel) override {
        local::push_to_client(conn, std::string(buffer, buffer + size));
    }

    ~LocalServer() override {
        running = false;
        local::reset();
    }

   private:
    std::function<void(HSteamNetConnection, std::string)> process_message_cb;
    std::function<void(HSteamNetConnection)> onClientDisconnect;
};

}  // namespace internal
}  // namespace network
