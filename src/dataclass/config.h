
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
