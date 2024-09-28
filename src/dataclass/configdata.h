#pragma once

#include <map>
#include <string>
#include <variant>

#include "../components/is_progression_manager.h"
#include "../engine/bitset_utils.h"
#include "../engine/type_name.h"
#include "settings.h"
#include "upgrade_class.h"

struct UpgradeImpl;

struct ConfigData {
    std::vector<EntityType> forever_required;
    std::vector<EntityType> store_to_spawn;

    Mods this_hours_mods;

   private:
    UpgradeClassBitSet unlocked_upgrades = UpgradeClassBitSet().reset();
    std::map<UpgradeClass, int> reusable_counts;

    struct Data {
        float roundlength;
        float patiencemultiplier;
        float customerspawnmultiplier;
        float pisstimer;
        float vomitfreqmultiplier;
        float drinkcostmultiplier;
        float vomitamountmultiplier;
        float test;
        float maxdrinktime;
        float maxdwelltime;
        int maxnumorders;
        int numstorespawns;
        int bladdersize;
        int storererollprice;

        template<typename T>
        void set(const ConfigKey& key, const T&) {
            log_error("Setting value for {} but not supported for {}",
                      key_name(key), type_name<T>());
            return T();
        }

        template<>
        void set(const ConfigKey& key, const float& value) {
            switch (key) {
                case ConfigKey::RoundLength:
                    roundlength = value;
                    break;
                case ConfigKey::PatienceMultiplier:
                    patiencemultiplier = value;
                    break;
                case ConfigKey::CustomerSpawnMultiplier:
                    customerspawnmultiplier = value;
                    break;
                case ConfigKey::PissTimer:
                    pisstimer = value;
                    break;
                case ConfigKey::VomitFreqMultiplier:
                    vomitfreqmultiplier = value;
                    break;
                case ConfigKey::DrinkCostMultiplier:
                    drinkcostmultiplier = value;
                    break;
                case ConfigKey::VomitAmountMultiplier:
                    vomitamountmultiplier = value;
                    break;
                case ConfigKey::Test:
                    test = value;
                    break;
                case ConfigKey::MaxDrinkTime:
                    maxdrinktime = value;
                    break;
                case ConfigKey::MaxDwellTime:
                    maxdwelltime = value;
                    break;
                case ConfigKey::MaxNumOrders:
                case ConfigKey::NumStoreSpawns:
                case ConfigKey::BladderSize:
                case ConfigKey::StoreRerollPrice:
                    log_error(
                        "Setting value for {} but not supported for float",
                        key_name(key));
                    break;
            }
        }

        template<>
        void set(const ConfigKey& key, const int& value) {
            switch (key) {
                case ConfigKey::RoundLength:
                case ConfigKey::PatienceMultiplier:
                case ConfigKey::CustomerSpawnMultiplier:
                case ConfigKey::PissTimer:
                case ConfigKey::VomitFreqMultiplier:
                case ConfigKey::DrinkCostMultiplier:
                case ConfigKey::VomitAmountMultiplier:
                case ConfigKey::Test:
                case ConfigKey::MaxDrinkTime:
                case ConfigKey::MaxDwellTime:
                    log_error("Setting value for {} but not supported for int",
                              key_name(key));
                    break;
                case ConfigKey::MaxNumOrders:
                    maxnumorders = value;
                    break;
                case ConfigKey::NumStoreSpawns:
                    numstorespawns = value;
                    break;
                case ConfigKey::BladderSize:
                    bladdersize = value;
                    break;
                case ConfigKey::StoreRerollPrice:
                    storererollprice = value;
                    break;
            }
        }

        template<typename T>
        [[nodiscard]] T get(const ConfigKey& key) const {
            log_error("Fetching value for {} but not supported for {}",
                      key_name(key), type_name<T>());
            return T();
        }

        template<>
        [[nodiscard]] float get(const ConfigKey& key) const {
            switch (key) {
                case ConfigKey::RoundLength:
                    return roundlength;
                case ConfigKey::PatienceMultiplier:
                    return patiencemultiplier;
                case ConfigKey::CustomerSpawnMultiplier:
                    return customerspawnmultiplier;
                case ConfigKey::PissTimer:
                    return pisstimer;
                case ConfigKey::VomitFreqMultiplier:
                    return vomitfreqmultiplier;
                case ConfigKey::DrinkCostMultiplier:
                    return drinkcostmultiplier;
                case ConfigKey::VomitAmountMultiplier:
                    return vomitamountmultiplier;
                case ConfigKey::Test:
                    return test;
                case ConfigKey::MaxDrinkTime:
                    return maxdrinktime;
                case ConfigKey::MaxDwellTime:
                    return maxdwelltime;
                case ConfigKey::MaxNumOrders:
                case ConfigKey::NumStoreSpawns:
                case ConfigKey::BladderSize:
                case ConfigKey::StoreRerollPrice:
                    break;
            }
            log_error("Fetching value for {} but not supported for float",
                      key_name(key));
            return 0.f;
        }

        template<>
        [[nodiscard]] int get(const ConfigKey& key) const {
            switch (key) {
                case ConfigKey::RoundLength:
                case ConfigKey::PatienceMultiplier:
                case ConfigKey::CustomerSpawnMultiplier:
                case ConfigKey::PissTimer:
                case ConfigKey::VomitFreqMultiplier:
                case ConfigKey::DrinkCostMultiplier:
                case ConfigKey::VomitAmountMultiplier:
                case ConfigKey::Test:
                case ConfigKey::MaxDrinkTime:
                case ConfigKey::MaxDwellTime:
                    break;
                case ConfigKey::MaxNumOrders:
                    return maxnumorders;
                case ConfigKey::NumStoreSpawns:
                    return numstorespawns;
                case ConfigKey::BladderSize:
                    return bladdersize;
                case ConfigKey::StoreRerollPrice:
                    return storererollprice;
            }
            log_error("Fetching value for {} but not supported for int",
                      key_name(key));
            return 0.f;
        }

        [[nodiscard]] bool contains(const ConfigKey& key) const {
            switch (key) {
                case ConfigKey::Test:
                case ConfigKey::RoundLength:
                case ConfigKey::MaxNumOrders:
                case ConfigKey::PatienceMultiplier:
                case ConfigKey::CustomerSpawnMultiplier:
                case ConfigKey::NumStoreSpawns:
                case ConfigKey::StoreRerollPrice:
                case ConfigKey::PissTimer:
                case ConfigKey::BladderSize:
                case ConfigKey::DrinkCostMultiplier:
                case ConfigKey::VomitFreqMultiplier:
                case ConfigKey::VomitAmountMultiplier:
                case ConfigKey::MaxDrinkTime:
                case ConfigKey::MaxDwellTime:
                    return true;
            }
            log_info("Config key {} but missing from data", key_name(key));
            return false;
        }

        template<typename S>
        void serialize(S& s) {
            s.value4b(roundlength);
            s.value4b(patiencemultiplier);
            s.value4b(customerspawnmultiplier);
            s.value4b(pisstimer);
            s.value4b(vomitfreqmultiplier);
            s.value4b(drinkcostmultiplier);
            s.value4b(vomitamountmultiplier);
            s.value4b(test);
            s.value4b(maxdrinktime);
            s.value4b(maxnumorders);
            s.value4b(numstorespawns);
            s.value4b(bladdersize);
            s.value4b(storererollprice);
        }
    };
    Data data;

    template<typename T>
    T modify(ConfigKey key, T input, Operation op, T value) const {
        switch (op) {
            case Operation::Add:
                return input + value;
                break;
            case Operation::Multiplier:
                return input * value;
                break;
            case Operation::Divide:
                return input / value;
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
        return data.get<T>(key);
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
        data.set(key, value);
    }

    [[nodiscard]] bool meets_prereq(const UpgradeClass& uc,
                                    const IsProgressionManager& ipm);

    [[nodiscard]] size_t count_missing_prereqs(const IsProgressionManager& ipm);

   public:
    template<typename T>
    [[nodiscard]] bool contains(const ConfigKey& key) const {
        return data.contains(key);
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

    [[nodiscard]] size_t num_unique_upgrades_unlocked() const {
        return unlocked_upgrades.count();
    }

    [[nodiscard]] bool has_upgrade_unlocked(const UpgradeClass& uc) const {
        return bitset_utils::test(unlocked_upgrades, uc);
    }

    void mark_upgrade_unlocked(const UpgradeClass& uc);
    void for_each_unlocked(
        const std::function<bitset_utils::ForEachFlow(UpgradeClass)> cb) const;

    std::vector<std::shared_ptr<UpgradeImpl>> get_possible_upgrades(
        const IsProgressionManager&);

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(unlocked_upgrades, bitsery::ext::StdBitset{});
        s.object(data);
    }
};
