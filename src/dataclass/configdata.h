#pragma once

#include <map>
#include <string>
#include <variant>

#include "settings.h"

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
};
