#pragma once

#include <map>
#include <string>
#include <variant>

#include "settings.h"

struct ConfigData {
    std::vector<EntityType> forever_required;
    std::vector<EntityType> store_to_spawn;

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
        // TODO :SPEED:  dont _need_ to do this in prod
        if (!contains<T>(key)) {
            log_error("get<>'ing key that does not exist for type {}",
                      key_name(key), type_name<T>());
        }

        auto vt = data.at(key);
        return std::get<T>(vt);
    }

    template<typename T>
    [[nodiscard]] T get(const ConfigKey& key) {
        // TODO :SPEED:  dont _need_ to do this in prod
        if (!contains<T>(key)) {
            log_error("get<>'ing key that does not exist for type {}",
                      key_name(key), type_name<T>());
        }

        auto vt = data.at(key);
        return std::get<T>(vt);
    }

    template<typename T>
    void set(const ConfigKey& key, T value) {
        data[key] = value;
    }
};
