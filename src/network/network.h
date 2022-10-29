

#pragma once

#include <steam/isteamnetworkingutils.h>
#include <steam/steamnetworkingsockets.h>
#include <steam/steamnetworkingtypes.h>

#include <chrono>
#include <thread>

#include "../assert.h"
#include "steam/isteamnetworkingsockets.h"

namespace network {

int quit(int rc) {
    if (rc == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    exit(rc);
}

void log(std::string msg) { std::cout << msg << std::endl; }

struct Client {
    SteamNetworkingIPAddr address;
    ISteamNetworkingSockets *interface;
    HSteamNetConnection connection;
    inline static Client *callback_instance;
    bool running = false;

    // Send to
    // {
    // // Anything else, just send it to the server and let them parse it
    // m_pInterface->SendMessageToConnection(
    // m_hConnection, cmd.c_str(), (uint32) cmd.length(),
    // k_nSteamNetworkingSend_Reliable, nullptr);
    // }

    Client(SteamNetworkingIPAddr addy) : address(addy) {}

    void startup() {
        interface = SteamNetworkingSockets();

        SteamNetworkingConfigValue_t opt;
        opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged,
                   (void *) Client::SteamNetConnectionStatusChangedCallback);
        connection = interface->ConnectByIPAddress(address, 1, &opt);
        if (connection == k_HSteamNetConnection_Invalid) {
            log(fmt::format("Failed to create connection"));
            return;
        }
        log("success connecting");
        running = true;
    }

    void process_message_string(std::string msg) {
        log(fmt::format("Client: {}", msg));
    }

    bool run() {
        if (!running) {
            return false;
        }

        auto poll_incoming_messages = [&]() {
            ISteamNetworkingMessage *incoming_msg = nullptr;
            int num_msgs = interface->ReceiveMessagesOnConnection(
                connection, &incoming_msg, 1);
            if (num_msgs == 0) return;
            if (num_msgs == -1) {
                M_ASSERT(false, "Failed checking for messages");
                return;
            }

            std::string cmd;
            cmd.assign((const char *) incoming_msg->m_pData,
                       incoming_msg->m_cbSize);
            incoming_msg->Release();
            this->process_message_string(cmd);
        };
        auto poll_connection_state_changes = [&]() {
            callback_instance = this;
            interface->RunCallbacks();
        };

        poll_incoming_messages();
        poll_connection_state_changes();

        return true;
    }

    void on_steam_network_connection_status_changed(
        SteamNetConnectionStatusChangedCallback_t *info) {
        M_ASSERT(info->m_hConn == connection ||
                     connection == k_HSteamNetConnection_Invalid,
                 "Connection Status Error");
        log("client on stream network connection status changed");

        switch (info->m_info.m_eState) {
            case k_ESteamNetworkingConnectionState_None:
                // NOTE: We will get callbacks here when we destroy connections.
                // You can ignore these.
                break;

            case k_ESteamNetworkingConnectionState_ClosedByPeer:
            case k_ESteamNetworkingConnectionState_ProblemDetectedLocally: {
                running = false;
                // Print an appropriate message
                if (info->m_eOldState ==
                    k_ESteamNetworkingConnectionState_Connecting) {
                    // Note: we could distinguish between a timeout, a rejected
                    // connection, or some other transport problem.
                    log(
                        fmt::format("We sought the remote host, yet our "
                                    "efforts were met with defeat.  ({})",
                                    info->m_info.m_szEndDebug));
                } else if (
                    info->m_info.m_eState ==
                    k_ESteamNetworkingConnectionState_ProblemDetectedLocally) {
                    log(fmt::format(
                        "Alas, troubles beset us; we have lost contact with "
                        "the host.  ({})",
                        info->m_info.m_szEndDebug));
                } else {
                    // NOTE: We could check the reason code for a normal
                    // disconnection
                    log(fmt::format("The host hath bidden us farewell.  ({})",
                                    info->m_info.m_szEndDebug));
                }

                // Clean up the connection.  This is important!
                // The connection is "closed" in the network sense, but
                // it has not been destroyed.  We must close it on our end, too
                // to finish up.  The reason information do not matter in this
                // case, and we cannot linger because it's already closed on the
                // other end, so we just pass 0's.
                interface->CloseConnection(info->m_hConn, 0, nullptr, false);
                connection = k_HSteamNetConnection_Invalid;
                break;
            }

            case k_ESteamNetworkingConnectionState_Connecting:
                // We will get this callback when we start connecting.
                // We can ignore this.
                break;

            case k_ESteamNetworkingConnectionState_Connected:
                log("Connected to server OK");
                break;

            default:
                // Silences -Wswitch
                break;
        }
    }

    static void SteamNetConnectionStatusChangedCallback(
        SteamNetConnectionStatusChangedCallback_t *pInfo) {
        callback_instance->on_steam_network_connection_status_changed(pInfo);
    }
};

struct Server {
    SteamNetworkingIPAddr address;
    ISteamNetworkingSockets *interface;
    HSteamListenSocket listen_sock;
    HSteamNetPollGroup poll_group;
    inline static Server *callback_instance;
    bool running = false;

    struct Client_t {
        std::string username;
    };
    std::map<HSteamNetConnection, Client_t> clients;

    Server(int port) {
        M_ASSERT(port > 0 && port < 65535, "invalid port");
        address.Clear();
        address.m_port = (unsigned short) port;
    }

    void connection_changed_callback(
        SteamNetConnectionStatusChangedCallback_t *info) {
        log("connection_changed_callback");
        char temp[1024];

        // What's the state of the connection?
        switch (info->m_info.m_eState) {
            case k_ESteamNetworkingConnectionState_None:
                // NOTE: We will get callbacks here when we destroy connections.
                // You can ignore these.
                break;
            case k_ESteamNetworkingConnectionState_ClosedByPeer:
            case k_ESteamNetworkingConnectionState_ProblemDetectedLocally: {
                // Ignore if they were not previously connected.  (If they
                // disconnected before we accepted the connection.)
                if (info->m_eOldState ==
                    k_ESteamNetworkingConnectionState_Connected) {
                    // Locate the client.  Note that it should have been found,
                    // because this is the only codepath where we remove clients
                    // (except on shutdown), and connection change callbacks are
                    // dispatched in queue order.
                    auto itClient = clients.find(info->m_hConn);
                    assert(itClient != clients.end());

                    // Select appropriate log messages
                    const char *pszDebugLogAction;
                    if (info->m_info.m_eState ==
                        k_ESteamNetworkingConnectionState_ProblemDetectedLocally) {
                        pszDebugLogAction = "problem detected locally";
                        sprintf(temp, "Alas, %s hath fallen into shadow.  (%s)",
                                itClient->second.username.c_str(),
                                info->m_info.m_szEndDebug);
                    } else {
                        // Note that here we could check the reason code to see
                        // if it was a "usual" connection or an "unusual" one.
                        pszDebugLogAction = "closed by peer";
                        sprintf(temp, "%s hath departed",
                                itClient->second.username.c_str());
                    }

                    // Spew something to our own log.  Note that because we put
                    // their nick as the connection description, it will show
                    // up, along with their transport-specific data (e.g. their
                    // IP address)
                    log(fmt::format("Connection %s %s, reason %d: %s\n",
                                    info->m_info.m_szConnectionDescription,
                                    pszDebugLogAction,
                                    info->m_info.m_eEndReason,
                                    info->m_info.m_szEndDebug));

                    clients.erase(itClient);

                    // Send a message so everybody else knows what happened
                    send_string_to_all(temp);
                } else {
                    M_ASSERT(
                        info->m_eOldState ==
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
                break;
            }

            case k_ESteamNetworkingConnectionState_Connecting: {
                // This must be a new connection
                M_ASSERT(clients.find(info->m_hConn) == clients.end(),
                         "Client already connected but shouldnt be");

                log(fmt::format("Connection request from %s",
                                info->m_info.m_szConnectionDescription));

                // A client is attempting to connect
                // Try to accept the connection.
                if (interface->AcceptConnection(info->m_hConn) != k_EResultOK) {
                    // This could fail.  If the remote host tried to connect,
                    // but then disconnected, the connection may already be half
                    // closed.  Just destroy whatever we have on our side.
                    interface->CloseConnection(info->m_hConn, 0, nullptr,
                                               false);
                    log(fmt::format(
                        "Can't accept connection. (It was already closed?)"));
                    break;
                }

                // Assign the poll group
                if (!interface->SetConnectionPollGroup(info->m_hConn,
                                                       poll_group)) {
                    interface->CloseConnection(info->m_hConn, 0, nullptr,
                                               false);
                    log("Failed to set poll group?");
                    break;
                }

                // Generate a random nick.  A random temporary nick
                // is really dumb and not how you would write a real chat
                // server. You would want them to have some sort of signon
                // message, and you would keep their client in a state of limbo
                // (connected, but not logged on) until them.  I'm trying to
                // keep this example code really simple.
                char nick[64];
                sprintf(nick, "BraveWarrior%d", 10000 + (rand() % 100000));

                // Send them a welcome message
                sprintf(
                    temp,
                    "Welcome, stranger.  Thou art known to us for now as '%s'; "
                    "upon thine command '/nick' we shall know thee otherwise.",
                    nick);
                send_string_to_client(info->m_hConn, temp);

                // Also send them a list of everybody who is already connected
                if (clients.empty()) {
                    send_string_to_client(info->m_hConn,
                                          "Thou art utterly alone.");
                } else {
                    sprintf(temp,
                            "%d companions greet you:", (int) clients.size());
                    for (auto &c : clients)
                        send_string_to_client(info->m_hConn,
                                              c.second.username.c_str());
                }

                // Let everybody else know who they are for now
                sprintf(temp,
                        "Hark!  A stranger hath joined this merry host.  For "
                        "now we shall call them '%s'",
                        nick);
                send_string_to_all(temp);

                // Add them to the client list, using std::map wacky syntax
                clients[info->m_hConn];
                // SetClientNick(pInfo->m_hConn, nick);
                break;
            }

            case k_ESteamNetworkingConnectionState_Connected:
                // We will get a callback immediately after accepting the
                // connection. Since we are the server, we can ignore this, it's
                // not news to us.
                break;

            default:
                // Silences -Wswitch
                break;
        }
    }

    void startup() {
        interface = SteamNetworkingSockets();

        SteamNetworkingConfigValue_t opt;
        opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged,
                   (void *) Server::SteamNetConnectionStatusChangedCallback);

        listen_sock = interface->CreateListenSocketIP(address, 1, &opt);
        if (listen_sock == k_HSteamListenSocket_Invalid) {
            log(fmt::format("Failed to listen on port {}", address.m_port));
            return;
        }
        poll_group = interface->CreatePollGroup();
        if (poll_group == k_HSteamNetPollGroup_Invalid) {
            log(fmt::format("(poll group) Failed to listen on port {}",
                            address.m_port));
            return;
        }
        log(fmt::format("Server listening on port"));

        running = true;
    }

    void send_string_to_client(HSteamNetConnection conn, std::string str) {
        interface->SendMessageToConnection(
            conn, str.c_str(), (uint32) str.size(),
            k_nSteamNetworkingSend_Reliable, nullptr);
    }

    void send_string_to_all(std::string str) {
        for (auto &c : clients) {
            send_string_to_client(c.first, str);
        }
    }

    void process_message_string(std::string msg) {
        log(fmt::format("Server: {}", msg));
    }

    bool run() {
        if (!running) {
            teardown();
            return false;
        }

        auto poll_incoming_messages = [&]() {
            ISteamNetworkingMessage *incoming_msg = nullptr;
            int num_msgs = interface->ReceiveMessagesOnPollGroup(
                poll_group, &incoming_msg, 1);
            if (num_msgs == 0) return;
            if (num_msgs == -1) {
                M_ASSERT(false, "Failed checking for messages");
                return;
            }

            std::string cmd;
            cmd.assign((const char *) incoming_msg->m_pData,
                       incoming_msg->m_cbSize);
            incoming_msg->Release();
            this->process_message_string(cmd);
        };
        auto poll_connection_state_changes = [&]() {
            Server::callback_instance = this;
            interface->RunCallbacks();
        };
        auto poll_local_user_input = []() {
            // used to handle '/quit'
        };

        poll_incoming_messages();
        poll_connection_state_changes();
        poll_local_user_input();
        return true;
    }

    void teardown() {
        for (auto it : clients) {
            send_string_to_client(it.first, "Server is shutting down.");
            interface->CloseConnection(it.first, 0, "server shutdown", true);
        }
        clients.clear();
        interface->CloseListenSocket(listen_sock);
        listen_sock = k_HSteamListenSocket_Invalid;
        interface->DestroyPollGroup(poll_group);
        poll_group = k_HSteamNetPollGroup_Invalid;
    }

    static void SteamNetConnectionStatusChangedCallback(
        SteamNetConnectionStatusChangedCallback_t *pInfo) {
        callback_instance->connection_changed_callback(pInfo);
    }
};

SteamNetworkingMicroseconds START_TIME;

static void log_debug(ESteamNetworkingSocketsDebugOutputType eType,
                      const char *pszMsg) {
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

enum State {
    s_None = 1 << 0,
    s_Host = 1 << 1,
    s_Client = 1 << 2,
} desired_role = s_None;

int run() {
    auto init_connections = []() {
#ifdef BUILD_WITHOUT_STEAM
        SteamDatagramErrMsg errMsg;
        if (!GameNetworkingSockets_Init(nullptr, errMsg)) {
            log(fmt::format("GameNetworkingSockets init failed {}", errMsg));
        }
#endif
    };

    auto shutdown_connections = []() {
#ifdef BUILD_WITHOUT_STEAM
        GameNetworkingSockets_Kill();
#endif
    };

    ///////////////////////////////////

    START_TIME = SteamNetworkingUtils()->GetLocalTimestamp();
    SteamNetworkingUtils()->SetDebugOutputFunction(
        k_ESteamNetworkingSocketsDebugOutputType_Msg, log_debug);

    // desired_role = s_Host;
    desired_role = s_Client;

    init_connections();

    if (desired_role & s_Host) {
        log("I'm the host");
        Server server(770);
        server.startup();
        while (server.run()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        };
    }

    if (desired_role & s_Client) {
        log("im the client");
        SteamNetworkingIPAddr address;
        address.ParseString("127.0.0.1");
        address.m_port = 770;
        Client client(address);
        client.startup();
        while (client.run()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    shutdown_connections();
    return 0;
}

}  // namespace network
