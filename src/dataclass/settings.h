

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

using ConfigValueType = std::variant<int, bool, float, EntityType>;

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
};

struct ConfigValue {
    ConfigKey key;
    ConfigValueType value;
};

enum struct ConfigKeyType { Entity, Float, Bool, Int };

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

enum struct Operation { Multiplier, Set, Unlock };

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
    std::vector<UpgradeRequirement> prereqs;
    std::vector<EntityType> required_machines;
    int duration = -1;
};

static std::atomic_int UPGRADE_INSTANCE_ID_GEN = 0;
struct UpgradeInstance {
    int id;
    Upgrade parent_copy;

    UpgradeInstance() : id(UPGRADE_INSTANCE_ID_GEN++) {}
    explicit UpgradeInstance(const Upgrade& copy)
        : id(UPGRADE_INSTANCE_ID_GEN++), parent_copy(copy) {}
};
