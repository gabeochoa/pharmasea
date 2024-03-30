#pragma once

#include <bitset>

#include "../vendor_include.h"

enum struct UpgradeClass {
    LongerDay,
    UnlockToilet,
    BigBladders,
    BigCity,
    SmallTown,
    Champagne,
    HappyHour,
    Pitcher,
    MeAndTheBoys,
    MainStreet,
    Speakeasy,
    Mocktails,
    HeavyHanded,
    PottyProtocol,
};

using UpgradeClassBitSet = std::bitset<magic_enum::enum_count<UpgradeClass>()>;
