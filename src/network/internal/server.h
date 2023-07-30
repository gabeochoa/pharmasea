
#pragma once

#include <steam/isteamnetworkingutils.h>
#include <steam/steamnetworkingsockets.h>
#include <steam/steamnetworkingtypes.h>

#include <chrono>
#include <thread>

#include "../../engine/assert.h"
#include "../../engine/toastmanager.h"
#include "channel.h"
#include "steam/isteamnetworkingsockets.h"

namespace network {
namespace internal {

struct Client_t {
    int client_id;
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
    std::function<void(Client_t &, std::string)> process_message_cb;
    // TODO add setter and make private
    std::function<void(int)> onClientDisconnect;
    std::function<void(HSteamNetConnection, std::string,
                       InternalServerAnnouncement)>
        onSendClientAnnouncement;

    void set_process_message(
        std::function<void(Client_t &client, std::string)> cb) {
        process_message_cb = cb;
    }

    void set_announcement_cb(
        std::function<void(HSteamNetConnection, std::string,
                           InternalServerAnnouncement)>
            cb) {
        onSendClientAnnouncement = cb;
    }

    std::map<HSteamNetConnection, Client_t> clients;

    Server(int port) {
        VALIDATE(port > 0 && port < 65535, "invalid port");
        address.Clear();
        address.m_port = (unsigned short) port;
        callback_instance = this;
    }

    ~Server() {
        for (auto it : clients) {
            send_announcement_to_client(it.first, "server shutdown",
                                        InternalServerAnnouncement::Warn);
            interface->CloseConnection(it.first, 0, "server shutdown", true);
        }
        clients.clear();
        interface->CloseListenSocket(listen_sock);
        listen_sock = k_HSteamListenSocket_Invalid;
        interface->DestroyPollGroup(poll_group);
        poll_group = k_HSteamNetPollGroup_Invalid;
    }

    void send_announcement_to_client(HSteamNetConnection conn, std::string msg,
                                     InternalServerAnnouncement type) {
        if (onSendClientAnnouncement) onSendClientAnnouncement(conn, msg, type);
    }

    void send_announcement_to_all(
        std::string msg, InternalServerAnnouncement type,
        std::function<bool(Client_t &)> exclude = nullptr) {
        if (onSendClientAnnouncement) {
            for (auto &c : clients) {
                if (exclude && exclude(c.second)) continue;
                onSendClientAnnouncement(c.first, msg, type);
            }
        }
    }

    void send_message_to_all(
        const char *buffer, uint32 size,
        std::function<bool(Client_t &)> exclude = nullptr) {
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
            Client_t client = clients[incoming_msg->m_conn];
            incoming_msg->Release();

            //
            this->process_message_cb(client, cmd);
        };
        auto poll_connection_state_changes = [&]() {
            Server::callback_instance = this;
            interface->RunCallbacks();
        };
        poll_incoming_messages();
        poll_connection_state_changes();
        return true;
    }

    // TODO replace with tl::expected
    void startup() {
        interface = SteamNetworkingSockets();

        /// [connection int32] Upper limit of buffered pending bytes to be sent,
        /// if this is reached SendMessage will return k_EResultLimitExceeded
        /// Default is 512k (524288 bytes)
        SteamNetworkingUtils()->SetGlobalConfigValueInt32(
            k_ESteamNetworkingConfig_SendBufferSize, 1024 * 1024);

        /// [connection int32] Timeout value (in ms) to use after connection is
        /// established
        SteamNetworkingUtils()->SetGlobalConfigValueInt32(
            k_ESteamNetworkingConfig_TimeoutConnected, 10);

        SteamNetworkingConfigValue_t opt;
        opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged,
                   (void *) Server::SteamNetConnectionStatusChangedCallback);

        listen_sock = interface->CreateListenSocketIP(address, 1, &opt);
        if (listen_sock == k_HSteamListenSocket_Invalid) {
            log_warn("Failed to listen on port {}", address.m_port);
            return;
        }
        poll_group = interface->CreatePollGroup();
        if (poll_group == k_HSteamNetPollGroup_Invalid) {
            log_warn("(poll group) Failed to listen on port {}",
                     address.m_port);
            return;
        }
        log_info("Server listening on port");

        running = true;
    }

   private:
    void process_connection_ended(
        SteamNetConnectionStatusChangedCallback_t *info) {
        InternalServerAnnouncement annoucement_type;
        // Ignore if they were not previously connected.  (If they
        // disconnected before we accepted the connection.)
        if (info->m_eOldState == k_ESteamNetworkingConnectionState_Connected) {
            // Locate the client.  Note that it should have been found,
            // because this is the only codepath where we remove clients
            // (except on shutdown), and connection change callbacks are
            // dispatched in queue order.
            auto itClient = clients.find(info->m_hConn);
            assert(itClient != clients.end());

            std::string temp;
            // Select appropriate log messages
            const char *pszDebugLogAction;
            if (info->m_info.m_eState ==
                k_ESteamNetworkingConnectionState_ProblemDetectedLocally) {
                pszDebugLogAction = "problem detected locally";
                temp = fmt::format("Alas, {} hath fallen into shadow.  ({})",
                                   itClient->second.client_id,
                                   info->m_info.m_szEndDebug);
                annoucement_type = InternalServerAnnouncement::Warn;
            } else {
                // Note that here we could check the reason code to see
                // if it was a "usual" connection or an "unusual" one.
                pszDebugLogAction = "closed by peer";
                temp = fmt::format("{}; {} hath departed", temp,
                                   itClient->second.client_id);
                annoucement_type = InternalServerAnnouncement::Warn;
            }

            // Spew something to our own log.  Note that because we put
            // their nick as the connection description, it will show
            // up, along with their transport-specific data (e.g. their
            // IP address)
            log_warn("Connection {} {}, reason {}: {}\n",
                     info->m_info.m_szConnectionDescription, pszDebugLogAction,
                     info->m_info.m_eEndReason, info->m_info.m_szEndDebug);

            // TODO change keepalive timeout to be lower?

            int client_id = (int) itClient->second.client_id;

            if (onClientDisconnect) onClientDisconnect(client_id);

            clients.erase(itClient);

            // Send a message so everybody else knows what happened
            send_announcement_to_all(temp, annoucement_type);

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

        std::string temp;

        // Generate a random nick.  A random temporary nick
        // is really dumb and not how you would write a real chat
        // server. You would want them to have some sort of signon
        // message, and you would keep their client in a state of limbo
        // (connected, but not logged on) until them.  I'm trying to
        // keep this example code really simple.
        int nick_id = 10000 + (rand() % 100000);

        // Send them a welcome message
        temp = fmt::format(
            "Welcome, stranger.  Thou art known to us for now as '{}'; "
            "upon thine command '/nick' we shall know thee otherwise.",
            nick_id);
        send_announcement_to_client(info->m_hConn, temp,
                                    InternalServerAnnouncement::Info);

        // Also send them a list of everybody who is already connected
        if (clients.empty()) {
            send_announcement_to_client(info->m_hConn,
                                        "Thou art utterly alone.",
                                        InternalServerAnnouncement::Info);
        } else {
            temp =
                fmt::format("{} companions greet you:", (int) clients.size());
            for (auto &c : clients)
                send_announcement_to_client(
                    info->m_hConn,
                    fmt::format("{}, your id is: {}", temp, c.second.client_id),
                    InternalServerAnnouncement::Info);
        }

        // Add them to the client list, using std::map wacky syntax
        clients[info->m_hConn];

        clients[info->m_hConn].client_id = nick_id;
        interface->SetConnectionName(info->m_hConn,
                                     fmt::format("{}", nick_id).c_str());

        // Let everybody else know who they are for now
        temp = fmt::format(
            "Hark!  A stranger hath joined this merry host.  For "
            "now we shall call them '{}'",
            nick_id);
        send_announcement_to_all(temp, InternalServerAnnouncement::Info,
                                 [&](const Client_t &client) {
                                     return client.client_id == nick_id;
                                 });
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
}  // namespace internal
}  // namespace network
