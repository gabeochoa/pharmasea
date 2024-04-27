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
    SippyCups,
    DownTheHatch,
    Jukebox,
    CantEvenTell,
    // TODO OneForYou - more tips but every customer also gives you a drink
    // which makes it ahrder to walk around or somethign
};

using UpgradeClassBitSet = std::bitset<magic_enum::enum_count<UpgradeClass>()>;
