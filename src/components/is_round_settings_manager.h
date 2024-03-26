
#pragma once

#include <variant>

#include "../upgrade_library.h"
#include "../vec_util.h"
#include "base_component.h"

struct IsRoundSettingsManager : public BaseComponent {
    enum UpgradeTimeOfDay {
        Unlock,
        Daily,
        Hour,
    };

    struct ConfigData {
        using ConfigValueType = std::variant<int, bool, float>;
        std::map<ConfigKey, ConfigValueType> data;

        template<typename T>
        [[nodiscard]] bool contains(const ConfigKey& key) const {
            if (!data.contains(key)) return false;
            auto vt = data.at(key);
            return std::holds_alternative<T>(vt);
        }

        template<typename T>
        [[nodiscard]] T get(const ConfigKey& key) const {
            auto vt = data.at(key);
            return std::get<T>(vt);
        }

        template<typename T>
        [[nodiscard]] T get(const ConfigKey& key) {
            auto vt = data.at(key);
            return std::get<T>(vt);
        }

        template<typename T>
        void set(const ConfigKey& key, T value) {
            data[key] = value;
        }

    } config;

    int ran_for_hour = -1;

    int num_unlocked;
    std::vector<std::string> unlocked_upgrades;
    std::vector<ActivityOutcome> activities;

    std::vector<std::string> active_upgrades;

    EntityTypeSet on_unlock_entities;
    EntityTypeSet on_daily_entities;
    EntityTypeSet on_hour_entities;

    void set_active_upgrade(const std::string& name) {
        if (is_upgrade_active(name)) return;
        active_upgrades.push_back(name);
    }
    void unset_active_upgrade(const std::string& name) {
        remove_if_matching(active_upgrades, name);
    }
    bool is_upgrade_active(const std::string& name) const {
        return vector::contains(active_upgrades, name);
    }

    [[nodiscard]] EntityTypeSet required_entities() const {
        return on_unlock_entities | on_daily_entities | on_hour_entities;
    }

    [[nodiscard]] bool has_upgrade_unlocked(const std::string& name) const {
        return vector::contains(unlocked_upgrades, name);
    }

    void apply_effects(const std::string& name, UpgradeEffects& effects) {
        set_active_upgrade(name);
        for (const UpgradeEffect& effect : effects) {
            apply_effect(effect);
        }
    }

    void unapply_effects(const std::string& name, UpgradeEffects& effects) {
        unset_active_upgrade(name);
        for (const UpgradeEffect& effect : effects) {
            unapply_effect(effect);
        }
    }

    void unlock_upgrade(const std::string& name) {
        if (has_upgrade_unlocked(name)) {
            log_error("upgrade unlock failed since its already unlocked {}",
                      name);
            return;
        }
        unlocked_upgrades.push_back(name);
        num_unlocked++;

        // TODO i still would like this as a single function but its not bad
        auto upgrade = fetch_upgrade(name);
        apply_effects(name, upgrade.on_unlock);
    }

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
    [[nodiscard]] bool contains(ConfigKey key) const {
        return config.contains<T>(key);
    }

    template<typename T>
    [[nodiscard]] T get(ConfigKey key) const {
        if (!contains<T>(key)) {
            log_error("get<{}> for {} key doesnt exist", type_name<T>(),
                      key_name(key));
        }
        return config.get<T>(key);
    }

    [[nodiscard]] bool meets_prereqs_for_upgrade(
        const std::string& name) const {
        auto upgrade = fetch_upgrade(name);
        for (const UpgradeRequirement& req : upgrade.prereqs) {
            if (!meets_prereq(req)) return false;
        }
        return true;
    }
    IsRoundSettingsManager() {
        // init config

        config.set<int>(ConfigKey::DayCount, 1);

        config.set<int>(ConfigKey::MaxNumOrders, 1);
        config.set<int>(ConfigKey::NumStoreSpawns, 5);
        config.set<int>(ConfigKey::BladderSize, 1);

        config.set<float>(ConfigKey::RoundLength, 100.f);
        config.set<float>(ConfigKey::PatienceMultiplier, 1.f);
        config.set<float>(ConfigKey::CustomerSpawnMultiplier, 1.f);
        config.set<float>(ConfigKey::DrinkCostMultiplier, 1.f);
        config.set<float>(ConfigKey::PissTimer, 2.5f);
        config.set<float>(ConfigKey::VomitFreqMultiplier, 1.0f);
        config.set<float>(ConfigKey::VomitAmountMultiplier, 1.0f);

        config.set<bool>(ConfigKey::UnlockedToilet, false);
        config.set<bool>(ConfigKey::HasCityMultiplier, false);

        /* TODO no more configs
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
                case ConfigKeyType::Activity:
                    break;
            }
        }
        */
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

   private:
    template<typename T>
    T unapply_operation(const Operation& op, T before, T value) {
        switch (op) {
            case Operation::Multiplier:
                return before / value;
            case Operation::Set:
                log_error("Unsetting isnt supported on {}", type_name<T>());
                break;
            case Operation::Custom:
                // ignore
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

    template<typename T>
    [[nodiscard]] bool check_value(const ConfigKey& key, T value) const {
        // Right now we only support bool and greater than
        // eventually we probably need to say what direction we need the current
        // value to be relative to the new one
        //
        // ie this upgrade only applies if you dont have more than 3 drinks
        // unlocked

        if constexpr (std::is_same_v<T, bool>) {
            return get<T>(key) == value;
        }

        /*
         * TODO
        if constexpr (std::is_same_v<T, EntityType>) {
            return bitset_utils::test(unlocked_entities, value);
        }

        if constexpr (std::is_same_v<T, Drink>) {
            return bitset_utils::test(unlocked_drinks, value);
        }
        */

        T current_value = get<T>(key);
        bool meets = current_value > value;

        log_info("check_value kv {} {} > {}?", key_name(key), current_value,
                 value);

        return meets;
    }

    [[nodiscard]] bool meets_prereq(const UpgradeRequirement& req) const {
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

    template<typename T>
    void fetch_and_apply(const ConfigKey& key, const Operation& op,
                         const ConfigValueType& value) {
        T before = get<T>(key);
        T nv = apply_operation<T>(op, before, std::get<T>(value));
        config.set<T>(key, nv);

        log_info("apply effect key: {} op {} before: {} after {}",
                 key_name(key), op_name(op), before, nv);
    }

    template<typename T>
    T apply_operation(const Operation& op, T before, T value) {
        switch (op) {
            case Operation::Multiplier:
                return before * value;
            case Operation::Set:
                return value;
            case Operation::Custom:
                log_error("{} isnt supported on {}",
                          magic_enum::enum_name<Operation>(op), type_name<T>());
                break;
        }
        return value;
    }

    [[nodiscard]] Upgrade fetch_upgrade(const std::string& name) const {
        if (!UpgradeLibrary::get().contains(name)) {
            log_error("Failed to find upgrade with name: {}", name);
        }
        return UpgradeLibrary::get().get(name);
    }

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value4b(num_unlocked);
        s.container(unlocked_upgrades, num_unlocked,
                    [](S& s2, std::string& str) { s2.text1b(str, 64); });

        s.container(active_upgrades, num_unlocked,
                    [](S& s2, std::string& str) { s2.text1b(str, 64); });
    }

   public:
    template<typename T>
    void fetch_and_apply_TEST_ONLY(const ConfigKey& key, const Operation& op,
                                   const ConfigValueType& value) {
        return fetch_and_apply<T>(key, op, value);
    }

    template<typename T>
    T unapply_operation_TEST_ONLY(const Operation& op, T before, T value) {
        return unapply_operation<T>(op, before, value);
    }

    template<typename T>
    T apply_operation_TEST_ONLY(const Operation& op, T before, T value) {
        return apply_operation<T>(op, before, value);
    }
};
