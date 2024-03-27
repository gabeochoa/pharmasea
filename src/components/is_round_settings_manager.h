
#pragma once

#include <variant>

#include "../dataclass/configdata.h"
#include "../dataclass/upgrades.h"
#include "../vec_util.h"
#include "base_component.h"

struct IsRoundSettingsManager : public BaseComponent {
    ConfigData config;

    std::vector<std::shared_ptr<UpgradeImpl>> selected_upgrades;

    IsRoundSettingsManager() {
        // init config
        {
            config.permanent_set<int>(ConfigKey::DayCount, 1);

            config.permanent_set<int>(ConfigKey::MaxNumOrders, 1);
            config.permanent_set<int>(ConfigKey::NumStoreSpawns, 5);
            config.permanent_set<int>(ConfigKey::BladderSize, 1);

            config.permanent_set<float>(ConfigKey::RoundLength, 100.f);
            config.permanent_set<float>(ConfigKey::PatienceMultiplier, 1.f);
            config.permanent_set<float>(ConfigKey::CustomerSpawnMultiplier,
                                        1.f);
            config.permanent_set<float>(ConfigKey::DrinkCostMultiplier, 1.f);
            config.permanent_set<float>(ConfigKey::PissTimer, 2.5f);
            config.permanent_set<float>(ConfigKey::VomitFreqMultiplier, 1.0f);
            config.permanent_set<float>(ConfigKey::VomitAmountMultiplier, 1.0f);

            config.permanent_set<bool>(ConfigKey::UnlockedToilet, false);
            config.permanent_set<bool>(ConfigKey::HasCityMultiplier, false);
        }
    }

    [[nodiscard]] bool is_upgrade_active(const UpgradeClass&) const {
        // TODO add support
        return true;
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

   private:
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

        T current_value = get<T>(key);
        bool meets = current_value > value;

        log_info("check_value kv {} {} > {}?", key_name(key), current_value,
                 value);

        return meets;
    }

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
    }
};
