

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
    UnlockedToilet,
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
    Entity,
    Drink,
    CustomerSpawn,
};

struct ConfigValue {
    ConfigKey key;
    ConfigValueType value;
};

enum struct ConfigKeyType { Activity, Drink, Entity, Float, Bool, Int };

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
            return ConfigKeyType::Bool;
        case ConfigKey::Entity:
            return ConfigKeyType::Entity;
        case ConfigKey::Drink:
            return ConfigKeyType::Drink;
        case ConfigKey::CustomerSpawn:
            return ConfigKeyType::Activity;
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
        std::cout << ("exception converting entity type: {}", e.what())
                  << std::endl;
    }
    return EntityType::Unknown;
}

inline Drink str_to_drink(const std::string& str) {
    try {
        return magic_enum::enum_cast<Drink>(str, magic_enum::case_insensitive)
            .value();
    } catch (std::exception e) {
        std::cout << ("exception converting drink type: {}", e.what())
                  << std::endl;
    }
    return Drink::coke;
}

inline bool can_have_key_as_prereq(ConfigKey key) {
    switch (key) {
        case ConfigKey::Test:
        case ConfigKey::RoundLength:
        case ConfigKey::MaxNumOrders:
        case ConfigKey::NumStoreSpawns:
        case ConfigKey::UnlockedToilet:
        case ConfigKey::PissTimer:
        case ConfigKey::BladderSize:
        case ConfigKey::HasCityMultiplier:
        case ConfigKey::DayCount:
        case ConfigKey::Entity:
        case ConfigKey::Drink:
            return true;
        case ConfigKey::PatienceMultiplier:
        case ConfigKey::CustomerSpawnMultiplier:
        case ConfigKey::DrinkCostMultiplier:
        case ConfigKey::VomitFreqMultiplier:
        case ConfigKey::VomitAmountMultiplier:
        case ConfigKey::CustomerSpawn:
            return false;
    }
    return false;
}

inline bool can_have_key_as_every_hour(ConfigKey key) {
    switch (key) {
        case ConfigKey::NumStoreSpawns:
        case ConfigKey::DayCount:
        case ConfigKey::Entity:
        case ConfigKey::Drink:
        case ConfigKey::HasCityMultiplier:
        case ConfigKey::UnlockedToilet:
        case ConfigKey::CustomerSpawnMultiplier:
        case ConfigKey::RoundLength:
        case ConfigKey::MaxNumOrders:
        case ConfigKey::Test:
        case ConfigKey::PatienceMultiplier:
        case ConfigKey::PissTimer:
        case ConfigKey::BladderSize:
        case ConfigKey::DrinkCostMultiplier:
        case ConfigKey::VomitFreqMultiplier:
        case ConfigKey::VomitAmountMultiplier:
            return false;
        case ConfigKey::CustomerSpawn:
            return true;
    }
    return false;
}

inline std::string_view key_name(ConfigKey key) {
    return magic_enum::enum_name<ConfigKey>(key);
}

inline std::string_view op_name(Operation key) {
    return magic_enum::enum_name<Operation>(key);
}
