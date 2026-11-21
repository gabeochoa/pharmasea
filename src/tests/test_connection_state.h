#pragma once

#include "../engine/assert.h"
#include "../network/connection_state.h"

namespace tests {

inline void test_connection_state() {
    using namespace network;

    VALIDATE(to_connection_state(k_ESteamNetworkingConnectionState_None) ==
                 ConnectionState::Idle,
             "None should map to Idle");
    VALIDATE(to_connection_state(
                 k_ESteamNetworkingConnectionState_Connecting) ==
                 ConnectionState::Connecting,
             "Connecting should map to Connecting");
    VALIDATE(to_connection_state(
                 k_ESteamNetworkingConnectionState_FindingRoute) ==
                 ConnectionState::Connecting,
             "FindingRoute should map to Connecting");
    VALIDATE(to_connection_state(k_ESteamNetworkingConnectionState_Connected) ==
                 ConnectionState::Connected,
             "Connected should map to Connected");
    VALIDATE(to_connection_state(
                 k_ESteamNetworkingConnectionState_ClosedByPeer) ==
                 ConnectionState::Closed,
             "ClosedByPeer should map to Closed");
    VALIDATE(to_connection_state(
                 k_ESteamNetworkingConnectionState_ProblemDetectedLocally) ==
                 ConnectionState::Failed,
             "ProblemDetectedLocally should map to Failed");
    VALIDATE(to_connection_state(k_ESteamNetworkingConnectionState_Dead) ==
                 ConnectionState::Closed,
             "Dead should map to Closed");
}

}  // namespace tests
