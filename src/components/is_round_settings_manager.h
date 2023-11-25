
#pragma once

#include <bitsery/ext/std_map.h>
using StdMap = bitsery::ext::StdMap;
//

#include <exception>
#include <map>

#include "../config_key_library.h"
#include "../dataclass/ingredient.h"
#include "../dataclass/settings.h"
#include "../engine/bitset_utils.h"
#include "../engine/type_name.h"
#include "../entity_type.h"
#include "../upgrade_library.h"
#include "../vec_util.h"
#include "base_component.h"

inline std::string_view key_name(ConfigKey key) {
    return magic_enum::enum_name<ConfigKey>(key);
}

inline std::string_view op_name(Operation key) {
    return magic_enum::enum_name<Operation>(key);
}

namespace round_settings {
struct Config {
    std::map<ConfigKey, float> floats;
    std::map<ConfigKey, int> ints;
    std::map<ConfigKey, bool> bools;

    template<typename T>
    bool contains(ConfigKey key) const {
        if constexpr (std::is_same_v<T, float>) {
            return floats.contains(key);
        } else if constexpr (std::is_same_v<T, int>) {
            return ints.contains(key);
        } else if constexpr (std::is_same_v<T, bool>) {
            return bools.contains(key);
        } else if constexpr (std::is_same_v<T, EntityType>) {
            // TODO i think we need this for prereqs
            return true;
        } else if constexpr (std::is_same_v<T, Drink>) {
            // TODO i think we need this for prereqs
            return true;
        }
        log_warn(
            "IRSM:: contains value from config no match for {} with type "
            "{}",
            key_name(key), type_name<T>());
        throw std::runtime_error("Fetching key for invalid type");
    }

    template<typename T>
    T get(ConfigKey key) const {
        if constexpr (std::is_same_v<T, float>) {
            return floats.at(key);
        } else if constexpr (std::is_same_v<T, int>) {
            return ints.at(key);
        } else if constexpr (std::is_same_v<T, bool>) {
            return bools.at(key);
        } else if constexpr (std::is_same_v<T, EntityType>) {
            return EntityType::Unknown;
        } else if constexpr (std::is_same_v<T, Drink>) {
            return Drink::coke;
        }
        log_warn("IRSM:: get value from config no match for {} with type {}",
                 key_name(key), type_name<T>());
        throw std::runtime_error("Setting key for invalid type");
    }

    template<typename T>
    void set(ConfigKey key, T value) {
        if constexpr (std::is_same_v<T, float>) {
            floats[key] = value;
            return;
        } else if constexpr (std::is_same_v<T, int>) {
            ints[key] = value;
            return;
        } else if constexpr (std::is_same_v<T, bool>) {
            bools[key] = value;
            return;
        } else if constexpr (std::is_same_v<T, EntityType>) {
            return;
        } else if constexpr (std::is_same_v<T, Drink>) {
            return;
        }
        log_warn("IRSM:: set value from config no match for {} with type {}",
                 key_name(key), type_name<T>());
        throw std::runtime_error("Setting key for invalid type");
    }

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(floats, StdMap{magic_enum::enum_count<ConfigKey>()},
              [](S& sv, ConfigKey& key, float value) {
                  sv.value4b(key);
                  sv.value4b(value);
              });

        s.ext(ints, StdMap{magic_enum::enum_count<ConfigKey>()},
              [](S& sv, ConfigKey& key, int value) {
                  sv.value4b(key);
                  sv.value4b(value);
              });

        s.ext(bools, StdMap{magic_enum::enum_count<ConfigKey>()},
              [](S& sv, ConfigKey& key, bool value) {
                  sv.value4b(key);
                  sv.value1b(value);
              });
    }
};

}  // namespace round_settings

struct IsRoundSettingsManager : public BaseComponent {
    struct EntityToSpawn {
        EntityType type;
        bool free = false;
    };

    round_settings::Config config;

    /*
     Loaded => this upgrade is in the library
     Unlocked => you chose this upgrade at some point
     Enabled => its active at some point today
     Active => the effects are currently going
     Invactive => no longer changing anything
     Deleted / Disabled

     ============ ============ ============ ============

     Start of Day



     Mid Day
        apply temporary hour effects


     End of Day
        reduce_duration
        unapply_expired_upgrades
        remove_from_active

     */

    EntityTypeSet unlocked_entities;
    DrinkSet unlocked_drinks;

    std::vector<EntityType> required_entities;

    std::vector<EntityToSpawn> entities_to_spawn;

    // TODO combine these two
    std::vector<std::string> upgrades_applied;
    std::map<int, UpgradeInstance> temp_upgrades_applied;

    IsRoundSettingsManager() {
        for (const auto& pair : ConfigValueLibrary::get()) {
            const ConfigValue& config_value = pair.second;
            const auto type = get_type(config_value.key);
            switch (type) {
                case ConfigKeyType::Entity:
                    config.set(config_value.key,
                               std::get<EntityType>(config_value.value));
                    break;
                case ConfigKeyType::Drink:
                    config.set(config_value.key,
                               std::get<Drink>(config_value.value));
                    break;
                case ConfigKeyType::Float:
                    config.set(config_value.key,
                               std::get<float>(config_value.value));
                    break;
                case ConfigKeyType::Bool:
                    config.set(config_value.key,
                               std::get<bool>(config_value.value));
                    break;
                case ConfigKeyType::Int:
                    config.set(config_value.key,
                               std::get<int>(config_value.value));
                    break;
            }
        }
    }

    virtual ~IsRoundSettingsManager() {}

    template<typename T>
    [[nodiscard]] T get_for_init(ConfigKey key) const {
        if (!config.contains<T>(key)) {
            log_error("get_for_init<{}> for {} key doesnt exist",
                      type_name<T>(), key_name(key));
        }
        return config.get<T>(key);
    }

    template<typename T>
    [[nodiscard]] T get_with_default(ConfigKey key, T default_value) const {
        return contains<T>(key) ? get<T>(key) : default_value;
    }

    template<typename T>
    [[nodiscard]] T get(ConfigKey key) const {
        if (!contains<T>(key)) {
            log_error("get<{}> for {} key doesnt exist", type_name<T>(),
                      key_name(key));
        }
        return config.get<T>(key);
    }

    template<typename T>
    [[nodiscard]] bool contains(ConfigKey key) const {
        return config.contains<T>(key);
    }

    template<typename T>
    T apply_operation(const Operation& op, T before, T value) {
        switch (op) {
            case Operation::Multiplier:
                return before * value;
            case Operation::Set:
                return value;
            case Operation::Unlock:
                log_error("Unlock isnt supported on {}", type_name<T>());
        }
        return value;
    }

    template<>
    EntityType apply_operation(const Operation& op, EntityType,
                               EntityType value) {
        switch (op) {
            case Operation::Multiplier:
                log_error("Multiplier isnt supported on EntityType");
                break;
            case Operation::Set:
                log_error("Set isnt supported on EntityType");
                break;
            case Operation::Unlock:
                bitset_utils::set(unlocked_entities, value);
                entities_to_spawn.push_back(EntityToSpawn{value});
                break;
        }
        return value;
    }

    template<>
    Drink apply_operation(const Operation& op, Drink, Drink value) {
        switch (op) {
            case Operation::Multiplier:
                log_error("Multiplier isnt supported on EntityType");
                break;
            case Operation::Set:
                log_error("Set isnt supported on EntityType");
                break;
            case Operation::Unlock:
                bitset_utils::set(unlocked_drinks, value);
                break;
        }
        return value;
    }

    template<typename T>
    void fetch_and_apply(const ConfigKey& key, const Operation& op,
                         const ConfigValueType& value) {
        T before = get<T>(key);
        T nv = apply_operation<T>(op, before, std::get<T>(value));
        config.set<T>(key, nv);

        log_info("apply effect key: {} op {} before: {} after {}",
                 key_name(key), op_name(op), before, nv);
    }

    void apply_effect(const UpgradeEffect& effect) {
        ConfigKeyType ckt = get_type(effect.name);

        switch (ckt) {
            case ConfigKeyType::Entity: {
                fetch_and_apply<EntityType>(effect.name, effect.operation,
                                            effect.value);
            } break;
            case ConfigKeyType::Drink: {
                fetch_and_apply<Drink>(effect.name, effect.operation,
                                       effect.value);
            } break;
            case ConfigKeyType::Float: {
                fetch_and_apply<float>(effect.name, effect.operation,
                                       effect.value);
            } break;
            case ConfigKeyType::Bool: {
                fetch_and_apply<bool>(effect.name, effect.operation,
                                      effect.value);
            } break;
            case ConfigKeyType::Int: {
                fetch_and_apply<int>(effect.name, effect.operation,
                                     effect.value);
            } break;
        }
    }

    [[nodiscard]] bool is_temporary_upgrade(const Upgrade& upgrade) const {
        return upgrade.duration > 0;
    }

    void apply_upgrade(const Upgrade& upgrade) {
        log_info("Applying upgrade {}", upgrade.name);
        upgrades_applied.push_back(upgrade.name);
        num_applied++;

        if (is_temporary_upgrade(upgrade)) {
            UpgradeInstance instance = UpgradeInstance(upgrade);
            temp_upgrades_applied[instance.id] = instance;
        }

        for (const UpgradeEffect& effect : upgrade.effects) {
            apply_effect(effect);
        }

        for (const EntityType& et : upgrade.required_machines) {
            required_entities.push_back(et);
            entities_to_spawn.push_back(EntityToSpawn{et, true});
        }
    }

    void apply_upgrade_by_name(const std::string& name) {
        if (!UpgradeLibrary::get().contains(name)) {
            // TODO for now just error out but eventually just warn
            log_error("Failed to find upgrade with name: {}", name);
        }
        apply_upgrade(UpgradeLibrary::get().get(name));
    }

    template<typename T>
    T unapply_operation(const Operation& op, T before, T value) {
        switch (op) {
            case Operation::Multiplier:
                return before / value;
            case Operation::Set:
                log_error("Unsetting isnt supported on {}", type_name<T>());
            case Operation::Unlock:
                log_error("Unlock isnt supported on {}", type_name<T>());
        }
        return value;
    }

    template<>
    EntityType unapply_operation(const Operation& op, EntityType,
                                 EntityType value) {
        switch (op) {
            case Operation::Multiplier:
                log_error("Multiplier isnt supported");
            case Operation::Set:
                log_error("Unsetting isnt supported");
            case Operation::Unlock:
                bitset_utils::reset(unlocked_entities, value);
                break;
        }
        return value;
    }

    template<>
    Drink unapply_operation(const Operation& op, Drink, Drink value) {
        switch (op) {
            case Operation::Multiplier:
                log_error("Multiplier isnt supported");
            case Operation::Set:
                log_error("Unsetting isnt supported");
            case Operation::Unlock:
                bitset_utils::reset(unlocked_drinks, value);
                break;
        }
        return value;
    }

    template<typename T>
    void fetch_and_unapply(const ConfigKey& key, const Operation& op,
                           const ConfigValueType& value) {
        T before = get<T>(key);
        T nv = unapply_operation<T>(op, before, std::get<T>(value));
        config.set<T>(key, nv);

        log_info("unapply effect key: {} op {} before: {} after {}",
                 key_name(key), op_name(op), before, nv);
    }

    void unapply_effect(const UpgradeEffect& effect) {
        ConfigKeyType ckt = get_type(effect.name);

        switch (ckt) {
            case ConfigKeyType::Entity: {
                fetch_and_unapply<EntityType>(effect.name, effect.operation,
                                              effect.value);
            } break;
            case ConfigKeyType::Drink: {
                fetch_and_unapply<Drink>(effect.name, effect.operation,
                                         effect.value);
            } break;
            case ConfigKeyType::Float: {
                fetch_and_unapply<float>(effect.name, effect.operation,
                                         effect.value);
            } break;
            case ConfigKeyType::Bool: {
                fetch_and_unapply<bool>(effect.name, effect.operation,
                                        effect.value);
            } break;
            case ConfigKeyType::Int: {
                fetch_and_unapply<int>(effect.name, effect.operation,
                                       effect.value);
            } break;
        }
    }

    void unapply_upgrade(const Upgrade& upgrade) {
        log_info("Unapplying upgrade {}", upgrade.name);
        if (!remove_if_matching(upgrades_applied, upgrade.name)) {
            log_error(
                "trying to remove, failed to find upgrade in applied upgrades");
        }
        num_applied--;

        for (const UpgradeEffect& effect : upgrade.effects) {
            unapply_effect(effect);
        }

        for (const EntityType& et : upgrade.required_machines) {
            // TODO if two upgrades have the same entity added,
            // we dont have a way to know
            // // maybe store the duration in there too
            remove_if_matching(required_entities, et);

            // TODO delete the free entity off the map
        }
    }

    void unapply_upgrade_by_name(const std::string& name) {
        if (!UpgradeLibrary::get().contains(name)) {
            // TODO for now just error out but eventually just warn
            log_error("Failed to find upgrade with name: {}", name);
        }
        const Upgrade& upgrade = UpgradeLibrary::get().get(name);
        unapply_upgrade(upgrade);
    }

    template<typename T>
    [[nodiscard]] bool check_value(const ConfigKey& key, T value) {
        // Right now we only support bool and greater than
        // eventually we probably need to say what direction we need the current
        // value to be relative to the new one
        //
        // ie this upgrade only applies if you dont have more than 3 drinks
        // unlocked

        if constexpr (std::is_same_v<T, bool>) {
            return get<T>(key) == value;
        }

        if constexpr (std::is_same_v<T, EntityType>) {
            return bitset_utils::test(unlocked_entities, value);
        }

        if constexpr (std::is_same_v<T, Drink>) {
            return bitset_utils::test(unlocked_drinks, value);
        }

        T current_value = get<T>(key);
        bool meets = current_value > value;

        log_info("check_value kv {} {} > {}?", key_name(key), current_value,
                 value);

        return meets;
    }

    [[nodiscard]] bool meets_prereq(const UpgradeRequirement& req) {
        ConfigKeyType ckt = get_type(req.name);
        switch (ckt) {
            case ConfigKeyType::Entity: {
                return check_value<EntityType>(req.name,
                                               std::get<EntityType>(req.value));
            } break;
            case ConfigKeyType::Drink: {
                return check_value<Drink>(req.name, std::get<Drink>(req.value));
            } break;
            case ConfigKeyType::Float: {
                return check_value<float>(req.name, std::get<float>(req.value));
            } break;
            case ConfigKeyType::Bool: {
                return check_value<bool>(req.name, std::get<bool>(req.value));
            } break;
            case ConfigKeyType::Int: {
                return check_value<int>(req.name, std::get<int>(req.value));
            } break;
        }
        return false;
    }

    [[nodiscard]] bool meets_prereqs_for_upgrade(const std::string& name) {
        if (!UpgradeLibrary::get().contains(name)) {
            log_warn("Failed to find upgrade with name: {}", name);
            return false;
        }
        Upgrade upgrade = UpgradeLibrary::get().get(name);

        for (const UpgradeRequirement& req : upgrade.prereqs) {
            if (!meets_prereq(req)) return false;
        }

        return true;
    }

    [[nodiscard]] bool already_applied_upgrade(const std::string& name) {
        // Why doesnt vector have .contains
        return std::find(upgrades_applied.begin(), upgrades_applied.end(),
                         name) != upgrades_applied.end();
    }

   private:
    int num_applied = 0;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value4b(num_applied);
        s.container(upgrades_applied, num_applied,
                    [](S& s2, std::string& str) { s2.text1b(str, 64); });
    }
};
