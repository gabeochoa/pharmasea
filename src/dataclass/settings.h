

#pragma once

#include <stdexcept>
#include <variant>

// We redefine the max here because the max keyboardkey is in the 300s
#undef MAGIC_ENUM_RANGE_MAX
#define MAGIC_ENUM_RANGE_MAX 400

#include <magic_enum/magic_enum.hpp>
//
#include "../engine/log.h"
#include "../engine/util.h"

using ConfigValueType = std::variant<int, bool, float>;

enum struct ConfigKey {
    RoundLength,
    MaxNumOrders,
    PatienceMultiplier,
    CustomerSpawnMultiplier,
    //
    NumStoreSpawns,
    //
    DrinkCostMultiplier,
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

struct ConfigValue {
    ConfigKey key;
    ConfigValueType value;
};

enum struct ConfigKeyType { Float, Bool, Int };

inline ConfigKeyType get_type(ConfigKey key) {
    switch (key) {
        case ConfigKey::RoundLength:
        case ConfigKey::PatienceMultiplier:
        case ConfigKey::CustomerSpawnMultiplier:
        case ConfigKey::PissTimer:
        case ConfigKey::CostMultiplier:
        case ConfigKey::VomitFreqMultiplier:
        case ConfigKey::DrinkCostMultiplier:
            return ConfigKeyType::Float;
        case ConfigKey::MaxNumOrders:
        case ConfigKey::NumStoreSpawns:
        case ConfigKey::BladderSize:
        case ConfigKey::VomitAmountMultiplier:
            return ConfigKeyType::Int;
        case ConfigKey::UnlockedToilet:
        case ConfigKey::HasCityMultiplier:
            return ConfigKeyType::Bool;
    }
    return ConfigKeyType::Float;
}

inline ConfigKey to_configkey(const std::string& str) {
    const auto converted_str = util::remove_underscores(str);
    const auto op = magic_enum::enum_cast<ConfigKey>(
        converted_str, magic_enum::case_insensitive);
    if (!op.has_value()) {
        log_error("Failed to convert {} from string to config key ", str);
    }
    return op.value();
}

enum struct Operation { Multiplier, Set };

inline Operation to_operation(const std::string& str) {
    const auto converted_str = util::remove_underscores(str);
    const auto op = magic_enum::enum_cast<Operation>(
        converted_str, magic_enum::case_insensitive);
    if (!op.has_value()) {
        log_error("Failed to convert {} from string to operation", str);
    }
    return op.value();
}

struct UpgradeEffect {
    ConfigKey name;
    Operation operation;
    ConfigValueType value;
};

struct UpgradeRequirement {
    ConfigKey name;
    ConfigValueType value;
};

struct Upgrade {
    std::string name;
    std::string flavor_text;
    std::string description;
    std::vector<UpgradeEffect> effects;
};
