
#pragma once

#include <bitsery/ext/std_map.h>
using StdMap = bitsery::ext::StdMap;
//

#include <exception>
#include <map>

#include "../config_key_library.h"
#include "../dataclass/settings.h"
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

using UpgradeInstance = Upgrade;

struct IsRoundSettingsManager : public BaseComponent {
    std::vector<EntityType> required_entities;
    std::vector<EntityType> entities_to_spawn;

    // TODO combine these two
    std::vector<std::string> upgrades_applied;
    std::map<std::string, UpgradeInstance> temp_upgrades_applied;

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
            }
            log_warn(
                "IRSM:: get value from config no match for {} with type {}",
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
            }
            log_warn(
                "IRSM:: set value from config no match for {} with type {}",
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
    } config;

    IsRoundSettingsManager() {
        for (const auto& pair : ConfigValueLibrary::get()) {
            const ConfigValue& config_value = pair.second;
            const auto type = get_type(config_value.key);
            switch (type) {
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

    void apply_upgrade(const std::string& name) {
        if (!UpgradeLibrary::get().contains(name)) {
            // TODO for now just error out but eventually just warn
            log_error("Failed to find upgrade with name: {}", name);
        }
        log_info("Applying upgrade {}", name);
        upgrades_applied.push_back(name);

        // TODO this forces a copy
        // we are okay with this in the duration case, but otherwise we probably
        // want this to be const
        Upgrade upgrade = UpgradeLibrary::get().get(name);

        // if the upgrade has a duration we need to also add a copy into the map
        // TODO the map only supports one of each at the moment

        if (upgrade.duration > 0) {
            temp_upgrades_applied[name] = UpgradeInstance(upgrade);
        }

        for (const UpgradeEffect& effect : upgrade.effects) {
            apply_effect(effect);
        }

        for (const EntityType& et : upgrade.required_machines) {
            required_entities.push_back(et);
            entities_to_spawn.push_back(et);
        }
    }

    template<typename T>
    T unapply_operation(const Operation& op, T before, T value) {
        switch (op) {
            case Operation::Multiplier:
                return before / value;
            case Operation::Set:
                log_error("Unsetting isnt supported");
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

    void unapply_upgrade(const std::string& name) {
        if (!UpgradeLibrary::get().contains(name)) {
            // TODO for now just error out but eventually just warn
            log_error("Failed to find upgrade with name: {}", name);
        }
        log_info("Unapplying upgrade {}", name);

        if (!remove_if_matching(upgrades_applied, name)) {
            log_error(
                "trying to remove, failed to find upgrade in applied upgrades");
        }

        Upgrade upgrade = UpgradeLibrary::get().get(name);
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

    template<typename T>
    [[nodiscard]] bool check_value(const ConfigKey& key, T value) {
        // TODO Right now we only support bool and greater than
        // eventually we probably need to say what direction we need the current
        // value to be relative to the new one
        //
        // ie this upgrade only applies if you dont have more than 3 drinks
        // unlocked

        if constexpr (std::is_same_v<T, bool>) {
            return get<T>(key) == value;
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
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        // TODO there wont be more than 10k upgrades right?
        s.container(upgrades_applied, 10000,
                    [](S& s2, std::string str) { s2.text1b(str, 64); });
    }
};
