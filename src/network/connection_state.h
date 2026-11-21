#pragma once

#include <steam/steamnetworkingtypes.h>

#include <string_view>

#include "magic_enum/magic_enum.hpp"

namespace network {

enum class ConnectionState : int {
    Idle = 0,
    Connecting = 1,
    Connected = 2,
    Closed = 3,
    Failed = 4,
};

[[nodiscard]] inline ConnectionState to_connection_state(
    ESteamNetworkingConnectionState state) {
    switch (state) {
        case k_ESteamNetworkingConnectionState_None:
            return ConnectionState::Idle;
        case k_ESteamNetworkingConnectionState_Connecting:
            return ConnectionState::Connecting;
        case k_ESteamNetworkingConnectionState_FindingRoute:
            return ConnectionState::Connecting;
        case k_ESteamNetworkingConnectionState_Connected:
            return ConnectionState::Connected;
        case k_ESteamNetworkingConnectionState_ClosedByPeer:
            return ConnectionState::Closed;
        case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
            return ConnectionState::Failed;
        case k_ESteamNetworkingConnectionState_FinWait:
            return ConnectionState::Closed;
        case k_ESteamNetworkingConnectionState_Linger:
            return ConnectionState::Closed;
        case k_ESteamNetworkingConnectionState_Dead:
            return ConnectionState::Closed;
        case k_ESteamNetworkingConnectionState__Force32Bit:
            return ConnectionState::Idle;
    }
}

[[nodiscard]] inline std::string_view connection_state_name(
    ConnectionState state) {
    return magic_enum::enum_name(state);
}

}  // namespace network
