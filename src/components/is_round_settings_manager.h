
#pragma once

#include <bitsery/ext/std_map.h>
using StdMap = bitsery::ext::StdMap;
//

#include <exception>
#include <map>

#include "../config_key_library.h"
#include "../dataclass/settings.h"
#include "../engine/type_name.h"
#include "../upgrade_library.h"
#include "base_component.h"

inline std::string_view key_name(ConfigKey key) {
    return magic_enum::enum_name<ConfigKey>(key);
}

inline std::string_view op_name(Operation key) {
    return magic_enum::enum_name<Operation>(key);
}

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

    [[nodiscard]] ConfigValueType get_vt(ConfigKey key) const {
        const auto ckname = std::string(magic_enum::enum_name<ConfigKey>(key));
        return ConfigValueLibrary::get().get(ckname).value;
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
        return std::get<T>(get_vt(key));
    }

    template<typename T>
    [[nodiscard]] bool contains(ConfigKey key) const {
        return config.contains<T>(key);
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
    }
};
