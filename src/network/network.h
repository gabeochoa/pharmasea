

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

const int RELIABLE = k_nSteamNetworkingSend_Reliable;
int VIRTUAL_PORT_LOCAL = 0;
int VIRTUAL_PORT_REMOTE = 0;
HSteamListenSocket LISTEN_SOCK;
HSteamNetConnection CONNECTION;
void send_message_to_peer(const char *message) {
    EResult r = SteamNetworkingSockets()->SendMessageToConnection(
        CONNECTION, message, (int) strlen(message) + 1, RELIABLE, nullptr);
    M_ASSERT(r == k_EResultOK, "Failed to send message");
}

void on_steam_net_connection_status_changed(
    SteamNetConnectionStatusChangedCallback_t *connection_info) {
    switch (connection_info->m_info.m_eState) {
        case k_ESteamNetworkingConnectionState_ClosedByPeer:
        case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
            log(fmt::format(
                "[%s] %s, reason %d: %s\n",
                connection_info->m_info.m_szConnectionDescription,
                (connection_info->m_info.m_eState ==
                         k_ESteamNetworkingConnectionState_ClosedByPeer
                     ? "closed by peer"
                     : "problem detected locally"),
                connection_info->m_info.m_eEndReason,
                connection_info->m_info.m_szEndDebug));

            // Close our end
            SteamNetworkingSockets()->CloseConnection(connection_info->m_hConn,
                                                      0, nullptr, false);

            if (CONNECTION == connection_info->m_hConn) {
                CONNECTION = k_HSteamNetConnection_Invalid;

                // In this example, we will bail the test whenever this happens.
                // Was this a normal termination?
                int rc = 0;
                if (rc ==
                        k_ESteamNetworkingConnectionState_ProblemDetectedLocally ||
                    connection_info->m_info.m_eEndReason !=
                        k_ESteamNetConnectionEnd_App_Generic)
                    rc = 1;  // failure
                quit(rc);
            } else {
                // Why are we hearing about any another connection?
                assert(false);
            }

            break;

        case k_ESteamNetworkingConnectionState_None:
            // Notification that a connection was destroyed.  (By us,
            // presumably.) We don't need this, so ignore it.
            break;

        case k_ESteamNetworkingConnectionState_Connecting:

            // Is this a connection we initiated, or one that we are receiving?
            if (LISTEN_SOCK != k_HSteamListenSocket_Invalid &&
                connection_info->m_info.m_hListenSocket == LISTEN_SOCK) {
                // Somebody's knocking
                // Note that we assume we will only ever receive a single
                // connection
                assert(CONNECTION ==
                       k_HSteamNetConnection_Invalid);  // not really a bug in
                                                        // this code, but a bug
                                                        // in the test

                log(fmt::format(
                    "[%s] Accepting\n",
                    connection_info->m_info.m_szConnectionDescription));
                CONNECTION = connection_info->m_hConn;
                SteamNetworkingSockets()->AcceptConnection(
                    connection_info->m_hConn);
            } else {
                // Note that we will get notification when our own connection
                // that we initiate enters this state.
                assert(CONNECTION == connection_info->m_hConn);
                log(fmt::format(
                    "[%s] Entered connecting state\n",
                    connection_info->m_info.m_szConnectionDescription));
            }
            break;

        case k_ESteamNetworkingConnectionState_FindingRoute:
            // P2P connections will spend a brief time here where they swap
            // addresses and try to find a route.
            log(fmt::format("[%s] finding route\n",
                            connection_info->m_info.m_szConnectionDescription));
            break;

        case k_ESteamNetworkingConnectionState_Connected:
            // We got fully connected
            assert(connection_info->m_hConn ==
                   CONNECTION);  // We don't initiate or accept any other
                                 // connections, so this should be out own
                                 // connection
            log(fmt::format("[%s] connected\n",
                            connection_info->m_info.m_szConnectionDescription));
            break;

        default:
            assert(false);
            break;
    }
}

struct Client {
    SteamNetworkingIPAddr address;
    Client(SteamNetworkingIPAddr addy) : address(addy) {}

    void run() {}
};
struct Server {
    SteamNetworkingIPAddr address;
    ISteamNetworkingSockets *interface;
    HSteamListenSocket listen_sock;
    HSteamNetPollGroup poll_group;

    struct Client_t {
        std::string username;
    };

    Server(int port) {
        M_ASSERT(port > 0 && port < 65535, "invalid port");
        address.Clear();
        address.m_port = (unsigned short) port;
    }

    void connection_changed_callback();

    void startup() {
        interface = SteamNetworkingSockets();

        SteamNetworkingConfigValue_t opt;
        opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged,
                   (void *) connection_changed_callback);

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
    }

    void tick() {
        poll_incoming_messages();
        poll_connection_state_changes();
        poll_local_user_input();
    }

    void teardown() {
        for (auto it : clients) {
            send_string_to_client(it.first, "Server is shutting down.");
            interface->CloseConnection(it.first, 0, "server shutdown", true;
        }
        clients.clear();
        interface->CloseListenSocket(listen_sock);
        listen_sock = k_HSteamListenSocket_Invalid;
        interface->DestroyPollGroup(poll_group);
        poll_group = k_HSteamNetPollGroup_Invalid;
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

    desired_role = s_Host;

    init_connections();

    if (desired_role & s_Host) {
        Server server(770);
        server.run();
    }

    if (desired_role & s_Client) {
        SteamNetworkingIPAddr address;
        address.ParseString("127.0.0.1");
        address.m_port = 770;
        Client client(address);
    }

    shutdown_connections();
}

}  // namespace network
