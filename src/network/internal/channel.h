
#pragma once

#include "steam/steamnetworkingtypes.h"

enum Channel {
    RELIABLE = k_nSteamNetworkingSend_Reliable,
    UNRELIABLE = k_nSteamNetworkingSend_Unreliable,
    UNRELIABLE_NO_DELAY = k_nSteamNetworkingSend_UnreliableNoDelay,
};
