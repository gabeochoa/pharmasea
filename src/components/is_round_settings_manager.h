
#pragma once

#include <variant>

#include "../dataclass/configdata.h"
#include "../dataclass/upgrades.h"
#include "../vec_util.h"
#include "base_component.h"

struct IsRoundSettingsManager : public BaseComponent {
    struct InteractiveSettings {
        // Turn tutorial off by default for now :)
        bool is_tutorial_active = false;

        friend bitsery::Access;
        template<typename S>
        void serialize(S& s) {
            s.value1b(is_tutorial_active);
        }
    } interactive_settings;

    ConfigData config;

    std::vector<std::shared_ptr<UpgradeImpl>> selected_upgrades;
    int ran_for_hour = -1;

    IsRoundSettingsManager() {
        // init config
        //

        magic_enum::enum_for_each<ConfigKey>([&](auto val) {
            constexpr ConfigKey key = val;
            switch (key) {
                case ConfigKey::Test:
                    break;
                case ConfigKey::RoundLength:
                    config.permanent_set<float>(ConfigKey::RoundLength, 100.f);
                    break;
                case ConfigKey::MaxNumOrders:
                    config.permanent_set<int>(ConfigKey::MaxNumOrders, 1);
                    break;
                case ConfigKey::PatienceMultiplier:
                    config.permanent_set<float>(ConfigKey::PatienceMultiplier,
                                                1.f);
                    break;
                case ConfigKey::CustomerSpawnMultiplier:
                    config.permanent_set<float>(
                        ConfigKey::CustomerSpawnMultiplier, 1.f);
                    break;
                case ConfigKey::NumStoreSpawns:
                    config.permanent_set<int>(ConfigKey::NumStoreSpawns, 10);
                    break;
                case ConfigKey::PissTimer:
                    config.permanent_set<float>(ConfigKey::PissTimer, 2.5f);
                    break;
                case ConfigKey::BladderSize:
                    config.permanent_set<int>(ConfigKey::BladderSize, 1);
                    break;
                case ConfigKey::DrinkCostMultiplier:
                    config.permanent_set<float>(ConfigKey::DrinkCostMultiplier,
                                                1.f);
                    break;
                case ConfigKey::VomitFreqMultiplier:
                    config.permanent_set<float>(ConfigKey::VomitFreqMultiplier,
                                                1.0f);
                    break;
                case ConfigKey::VomitAmountMultiplier:
                    config.permanent_set<float>(
                        ConfigKey::VomitAmountMultiplier, 1.0f);
                    break;
                case ConfigKey::MaxDrinkTime:
                    config.permanent_set<float>(ConfigKey::MaxDrinkTime, 1.0f);
                    break;
                case ConfigKey::MaxDwellTime:
                    config.permanent_set<float>(ConfigKey::MaxDwellTime, 5.0f);
                    break;
                case ConfigKey::StoreRerollPrice:
                    config.permanent_set<int>(ConfigKey::StoreRerollPrice, 50);
                    break;
            }
        });
    }

    [[nodiscard]] bool is_upgrade_active(const UpgradeClass& upgrade) const {
        switch (upgrade) {
            case UpgradeClass::LongerDay:
            case UpgradeClass::UnlockToilet:
            case UpgradeClass::BigBladders:
            case UpgradeClass::BigCity:
            case UpgradeClass::SmallTown:
            case UpgradeClass::Champagne:
            case UpgradeClass::Pitcher:
            case UpgradeClass::MeAndTheBoys:
            case UpgradeClass::MainStreet:
            case UpgradeClass::Speakeasy:
            case UpgradeClass::Mocktails:
            case UpgradeClass::HeavyHanded:
            case UpgradeClass::PottyProtocol:
            case UpgradeClass::SippyCups:
            case UpgradeClass::DownTheHatch:
            case UpgradeClass::Jukebox:
            case UpgradeClass::CantEvenTell:
                return true;
                break;
            case UpgradeClass::HappyHour:
                // TODO add support for upgrades that arent active 100% of the
                // time
                break;
        }
        return false;
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

    [[nodiscard]] bool has_upgrade_unlocked(const UpgradeClass& uc) const {
        return config.has_upgrade_unlocked(uc);
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
        s.object(config);
    }
};
