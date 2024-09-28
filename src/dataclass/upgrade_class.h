#pragma once

#include <bitset>

#include "../engine/constexpr_containers.h"
#include "../vendor_include.h"

enum struct UpgradeClass {
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

    // Reusables
    LongerDay,
};

using UpgradeClassBitSet = std::bitset<magic_enum::enum_count<UpgradeClass>()>;

constexpr std::array<UpgradeClass, 1> ReusableUpgrades = {{
    UpgradeClass::LongerDay,
}};

[[nodiscard]] constexpr bool upgrade_class_is_reusable(UpgradeClass uc) {
    return array_contains(ReusableUpgrades, uc);
}
