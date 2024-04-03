#pragma once

#include <map>
#include <string>
#include <variant>

#include "../engine/bitset_utils.h"
#include "settings.h"
#include "upgrade_class.h"

struct ConfigData {
    std::vector<EntityType> forever_required;
    std::vector<EntityType> store_to_spawn;

    Mods this_hours_mods;

    UpgradeClassBitSet unlocked_upgrades = UpgradeClassBitSet().reset();

   private:
    using ConfigValueType = std::variant<int, bool, float>;
    std::map<ConfigKey, ConfigValueType> data;

    template<typename T>
    T modify(ConfigKey key, T input, Operation op, T value) const {
        switch (op) {
            case Operation::Multiplier:
                return input * value;
                break;
            case Operation::Set:
            case Operation::Custom:
                log_error(
                    "Trying to fetch with hourly mods that have "
                    "unsupported operations for key: {}",
                    key_name(key));
                break;
        }
        return input;
    }

    template<typename T>
    [[nodiscard]] T raw_get(const ConfigKey& key) const {
        auto vt = data.at(key);
        return std::get<T>(vt);
    }

    template<typename T>
    [[nodiscard]] T underlying_fetch(const ConfigKey& key) const {
        // TODO :SPEED:  dont _need_ to do this in prod
        if (!contains<T>(key)) {
            log_error("get<>'ing key that does not exist for type {}",
                      key_name(key), type_name<T>());
        }
        // Real value
        T real_value = raw_get<T>(key);
        T new_value = real_value;

        // Modify the value based on the mods
        for (auto& mod : this_hours_mods) {
            if (mod.name != key) continue;

            new_value = modify<T>(key, new_value, mod.operation,
                                  std::get<T>(mod.value));
        }

        // return new value
        return new_value;
    }

    template<typename T>
    void set(const ConfigKey& key, T value) {
        data[key] = value;
    }

   public:
    template<typename T>
    [[nodiscard]] bool contains(const ConfigKey& key) const {
        if (!data.contains(key)) return false;
        auto vt = data.at(key);
        return std::holds_alternative<T>(vt);
    }

    template<typename T>
    [[nodiscard]] T get(const ConfigKey& key) const {
        return underlying_fetch<T>(key);
    }

    template<typename T>
    void permanent_set(const ConfigKey& key, T value) {
        set<T>(key, value);
    }

    template<typename T>
    void permanently_modify(const ConfigKey& key, Operation op, T value) {
        T input = raw_get<T>(key);
        T new_value = modify<T>(key, input, op, value);
        permanent_set<T>(key, new_value);
    }

    [[nodiscard]] bool has_upgrade_unlocked(const UpgradeClass& uc) const {
        return bitset_utils::test(unlocked_upgrades, uc);
    }

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(unlocked_upgrades, bitsery::ext::StdBitset{});
    }
};
