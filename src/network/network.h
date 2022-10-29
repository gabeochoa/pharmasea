

#pragma once

#include "client.h"
#include "server.h"

namespace network {

void log(std::string msg) { std::cout << msg << std::endl; }

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
    // desired_role = s_Client;

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
