

#pragma once

#include <variant>

enum struct ConfigKey {
    RoundLength,
    MaxNumOrders,
    PatienceMultiplier,
    CustomerSpawnMultiplier,
    //
    NumStoreSpawns,
    //
    UnlockedToilet,
    PissTimer,
    BladderSize,
    //
    HasCityMultiplier,
    CostMultiplier,
    //
    VomitFreqMultiplier,
    VomitAmountMultiplier,
    //
};

enum struct Operation { Multiplier, Set };

struct UpgradeEffect {
    ConfigKey name;
    Operation operation;
    std::variant<int, float, bool> value;
};

struct UpgradeRequirement {
    ConfigKey name;
    std::variant<int, float, bool> value;
};

struct Upgrade {
    std::string name;
    std::string flavor_text;
    std::string description;
    std::vector<UpgradeEffect> effects;
};
