
#pragma once

// We redefine the max here because the max keyboardkey is in the 300s
#undef MAGIC_ENUM_RANGE_MAX
#define MAGIC_ENUM_RANGE_MAX 400

#include <magic_enum/magic_enum.hpp>
//
#include <bitsery/ext/std_map.h>
using StdMap = bitsery::ext::StdMap;
//

#include <exception>
#include <map>

#include "../dataclass/settings.h"
#include "../engine/type_name.h"
#include "base_component.h"

struct IsRoundSettingsManager : public BaseComponent {
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
            // log_warn(
            // "IRSM:: contains value from config no match for {} with type "
            // "{}",
            // key, type_name<T>());
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
            // log_warn(
            // "IRSM:: get value from config no match for {} with type {}",
            // key, type_name<T>());
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
            // log_warn(
            // "IRSM:: set value from config no match for {} with type {}",
            // key, type_name<T>());
            throw std::runtime_error("Setting key for invalid type");
        }

        void init() {
            magic_enum::enum_for_each<ConfigKey>([&](ConfigKey key) {
                switch (key) {
                        // TODO i dont think this is read more than on init()
                        // need to find downstream users
                    case ConfigKey::RoundLength:
                        set<float>(key, 100.f);
                        break;
                    case ConfigKey::MaxNumOrders:
                        set<int>(key, 1);
                        break;
                    case ConfigKey::PatienceMultiplier:
                        set<float>(key, 1.f);
                        break;
                    case ConfigKey::CustomerSpawnMultiplier:
                        set<float>(key, 1.f);
                        break;
                    case ConfigKey::NumStoreSpawns:
                        set<int>(key, 5);
                        break;
                        //
                    case ConfigKey::PissTimer:
                        set<float>(key, 2.5f);
                        break;
                    case ConfigKey::BladderSize:
                        set<int>(key, 1);
                        break;
                    case ConfigKey::HasCityMultiplier:
                        set<bool>(key, false);
                        break;
                    case ConfigKey::CostMultiplier:
                        set<float>(key, 1.f);
                        break;
                        // TODO get_speed_for_entity
                    case ConfigKey::VomitFreqMultiplier:
                        set<float>(key, 1.f);
                        break;
                    case ConfigKey::VomitAmountMultiplier:
                        set<float>(key, 1.f);
                        break;
                    case ConfigKey::UnlockedToilet:
                        set<bool>(key, false);
                        break;
                }
            });
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

    IsRoundSettingsManager() { config.init(); }

    virtual ~IsRoundSettingsManager() {}

    inline std::string_view key_name(ConfigKey key) const {
        return magic_enum::enum_name<ConfigKey>(key);
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
        return config.contains<T>(key) ? config.get<T>(key) : default_value;
    }

    template<typename T>
    [[nodiscard]] T get(ConfigKey key) const {
        if (!config.contains<T>(key)) {
            log_error("get<{}> for {} key doesnt exist", type_name<T>(),
                      key_name(key));
        }
        return config.get<T>(key);
    }

    template<typename T>
    [[nodiscard]] T contains(ConfigKey key) const {
        return config.contains<T>(key);
    }

    template<typename T>
    void set(ConfigKey key, T value) {
        config.set<T>(key, value);
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.object(config);
    }
};
