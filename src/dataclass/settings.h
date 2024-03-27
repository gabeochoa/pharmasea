

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
#include "../entity_type.h"
#include "ingredient.h"

using ConfigValueType = std::variant<int, bool, float, EntityType, Drink>;

// TODO get_speed_for_entity
enum struct ConfigKey {
    Test,
    //

    RoundLength,
    MaxNumOrders,
    PatienceMultiplier,
    CustomerSpawnMultiplier,
    //
    NumStoreSpawns,
    //
    PissTimer,
    BladderSize,
    //
    HasCityMultiplier,
    DrinkCostMultiplier,
    //
    VomitFreqMultiplier,
    VomitAmountMultiplier,

    //
    DayCount,
    // Entity,
    // Drink,
    // CustomerSpawn,

    // TODO need some way to track already upgraded
    UnlockedToilet,
    UnlockedLongerDay,
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
        case ConfigKey::VomitFreqMultiplier:
        case ConfigKey::DrinkCostMultiplier:
        case ConfigKey::VomitAmountMultiplier:
        case ConfigKey::Test:
            return ConfigKeyType::Float;
        case ConfigKey::MaxNumOrders:
        case ConfigKey::NumStoreSpawns:
        case ConfigKey::BladderSize:
        case ConfigKey::DayCount:
            return ConfigKeyType::Int;
        case ConfigKey::UnlockedToilet:
        case ConfigKey::HasCityMultiplier:
        case ConfigKey::UnlockedLongerDay:
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

enum struct Operation { Multiplier, Set, Custom };

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

struct ConfigKeyValue {
    ConfigKey name;
    ConfigValueType value;
};
using UpgradeRequirement = ConfigKeyValue;
using ActivityOutcome = ConfigKeyValue;

using UpgradeActiveHours = std::bitset<100>;

using UpgradeEffects = std::vector<UpgradeEffect>;

using UpgradeHourlyEffect = std::pair<UpgradeActiveHours, UpgradeEffects>;
using UpgradeHourlyEffects = std::vector<UpgradeHourlyEffect>;

struct Upgrade {
    std::string name;
    std::string icon_name;
    std::string flavor_text;
    std::string description;
    std::vector<UpgradeRequirement> prereqs;
    std::vector<EntityType> required_machines;
    int duration;

    UpgradeEffects on_unlock;
    UpgradeEffects on_start_of_day;
    UpgradeHourlyEffects hourly_effects;
};

enum struct UpgradeType {
    None,
    Upgrade,
    Drink,
};
// inside preload.cpp
extern std::vector<UpgradeType> upgrade_rounds;

static std::atomic_int UPGRADE_INSTANCE_ID_GEN = 0;
struct UpgradeInstance {
    int id;
    Upgrade parent_copy;

    UpgradeInstance() : id(UPGRADE_INSTANCE_ID_GEN++) {}
    explicit UpgradeInstance(const Upgrade& copy)
        : id(UPGRADE_INSTANCE_ID_GEN++), parent_copy(copy) {}
};

inline EntityType str_to_entity_type(const std::string& str) {
    try {
        return magic_enum::enum_cast<EntityType>(str,
                                                 magic_enum::case_insensitive)
            .value();
    } catch (std::exception e) {
        log_error("exception converting entity type input:{} {}", str,
                  e.what());
    }
    return EntityType::Unknown;
}

inline Drink str_to_drink(const std::string& str) {
    try {
        return magic_enum::enum_cast<Drink>(str, magic_enum::case_insensitive)
            .value();
    } catch (std::exception e) {
        log_error("exception converting drink input:{} {}", str, e.what());
    }
    return Drink::coke;
}

inline std::string_view key_name(ConfigKey key) {
    return magic_enum::enum_name<ConfigKey>(key);
}

inline std::string_view op_name(Operation key) {
    return magic_enum::enum_name<Operation>(key);
}

struct UpgradeModification {
    ConfigKey name;
    Operation operation;
    ConfigValueType value;
};

using Mods = std::vector<UpgradeModification>;
