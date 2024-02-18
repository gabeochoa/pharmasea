
#include <algorithm>
#include <random>

#include "../components/has_timer.h"
#include "../components/is_progression_manager.h"
#include "../components/is_round_settings_manager.h"
#include "../entity_helper.h"
#include "magic_enum/magic_enum.hpp"
#include "system_manager.h"
#include "upgrade_system.h"

namespace system_manager {
namespace progression {

inline void skip_upgrade_visit() {
    GameState::get().transition_to_store();
    SystemManager::get().for_each_old([](Entity& e) {
        if (check_type(e, EntityType::Player)) {
            move_player_SERVER_ONLY(e, game::State::Store);
            return;
        }
    });
}

inline bool collect_drink_options(IsProgressionManager& ipm) {
    struct DrinkOption {
        Drink d = Drink::coke;
        size_t num_ing_needed = 0;
    };

    constexpr auto drinks = magic_enum::enum_values<Drink>();
    std::vector<DrinkOption> options;
    options.reserve(drinks.size());

    // Tries to unlock the next drink,
    // prioritizes options that are already using the same ingredients
    {
        for (auto drink : drinks) {
            // skip any already unlocked
            if (ipm.is_drink_unlocked(drink)) {
                log_info("already unlocked {} {}", drink,
                         magic_enum::enum_name<Drink>(drink));
                continue;
            }

            // This drink gets unlocked through upgrades instead
            // so dont show them in here
            if (needs_upgrade(drink)) continue;

            // We use get_req here because we want to also count any
            // prereqs that we need since those will spawn new machines/items as
            // well
            IngredientBitSet ing = get_req_ingredients_for_drink(drink);

            // TODO put these in utils somewhere so we can use when checking
            // overlap later

            // A:  000100100
            // B:  000100011
            // ~B: 111011100 (bitwise negation of B)
            // C:  000000100 (A & ~B)
            IngredientBitSet ing_needed = (ing & ~(ipm.enabled_ingredients()));
            size_t num_needed = ing_needed.count();

            // add to options in sorted order
            {
                options.push_back(DrinkOption{drink, num_needed});
                if (options.size() <= 1) continue;
                auto it = options.end() - 1;
                while (it != options.begin() &&
                       (it - 1)->num_ing_needed > it->num_ing_needed) {
                    std::iter_swap(it - 1, it);
                    --it;
                }
            }
        }
    }
    // TODO add a little randomness
    // TODO grab the rng engine from map
    // auto rng = std::default_random_engine{};
    // std::shuffle(std::begin(options), std::end(options), rng);

    if (options.size() < 2) {
        // No more options so just go direct to the store
        return false;
    }

    log_info("possible options");
    for (auto d_o : options) {
        log_info("{} {} : {}", d_o.d, magic_enum::enum_name<Drink>(d_o.d),
                 d_o.num_ing_needed);
    }

    ipm.drinkOption1 = options[0].d;
    ipm.drinkOption2 = options[1].d;

    log_info("got options for progression {} and {}", ipm.drinkOption1,
             ipm.drinkOption2);
    return true;
}

inline bool collect_upgrade_options(Entity& entity) {
    IsProgressionManager& ipm = entity.get<IsProgressionManager>();
    IsRoundSettingsManager& irsm = entity.get<IsRoundSettingsManager>();

    std::vector<Upgrade> possible_upgrades;

    log_info("num upgrades without filters : {}", UpgradeLibrary::get().size());

    for (const auto& kv : UpgradeLibrary::get()) {
        const Upgrade& upgrade = kv.second;

        if (irsm.has_upgrade_unlocked(upgrade.name)) continue;

        bool meets_preq = irsm.meets_prereqs_for_upgrade(upgrade.name);
        if (!meets_preq) continue;

        possible_upgrades.push_back(upgrade);
    }

    log_info("num upgrades with filters : {}", possible_upgrades.size());

    /* TODO
    // Choose the simpler upgrades first
    std::sort(possible_upgrades.begin(), possible_upgrades.end(),
              [&](auto it, auto it2) {
                  return it.effects.size() < it2.effects.size();
              });
              */

    if (possible_upgrades.size() < 2) {
        // No more options so just go direct to the store
        return false;
    }

    ipm.upgradeOption1 = possible_upgrades[0].name;
    ipm.upgradeOption2 = possible_upgrades[1].name;

    log_info(" The two options we got are {} and {}", ipm.upgradeOption1,
             ipm.upgradeOption2);
    return true;
}

inline void collect_progression_options(Entity& entity, float) {
    if (entity.is_missing<IsProgressionManager>()) return;
    IsProgressionManager& ipm = entity.get<IsProgressionManager>();

    // we already got the options and cached them...
    if (ipm.collectedOptions) return;

    switch (ipm.upgrade_type()) {
        case UpgradeType::None: {
            // If we arent in an upgrade round just go directly to planning
            log_info("not an upgrade round see ya next time");
            ipm.next_round();
            skip_upgrade_visit();
            return;
        } break;
        case UpgradeType::Drink: {
            bool got_drink_options = collect_drink_options(ipm);
            if (!got_drink_options) {
                log_info("could'nt get enough drink options");
                skip_upgrade_visit();
                // ipm.isUpgradeRound = false;
                return;
            }
        } break;
        case UpgradeType::Upgrade: {
            bool got_upgrade_options = collect_upgrade_options(entity);
            if (!got_upgrade_options) {
                skip_upgrade_visit();
                log_info("could'nt get enough upgrade options");
                // ipm.isUpgradeRound = false;
                return;
            }
        } break;
    }

    ipm.collectedOptions = true;
}

inline void update_upgrade_variables() {
    Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
    IsRoundSettingsManager& irsm = sophie.get<IsRoundSettingsManager>();
    // IsProgressionManager& ipm = sophie.get<IsProgressionManager>();

    // Apply activity outcomes,
    upgrade::execute_activites(irsm, upgrade::UpgradeTimeOfDay::Unlock,
                               irsm.activities);

    magic_enum::enum_for_each<ConfigKey>([&](auto val) {
        constexpr ConfigKey key = val;

        switch (key) {
            case ConfigKey::RoundLength: {
                HasTimer& hasTimer = sophie.get<HasTimer>();
                hasTimer.set_total_round_time(
                    irsm.get<float>(ConfigKey::RoundLength));

            } break;
            case ConfigKey::Drink: {
                // bitset_utils::for_each_enabled_bit(
                // irsm.unlocked_drinks, [&](size_t index) {
                // Drink drink = magic_enum::enum_value<Drink>(index);
                // ipm.unlock_drink(drink);
                // });
                // irsm.unlocked_drinks.reset();
            } break;
            case ConfigKey::Test:
            case ConfigKey::MaxNumOrders:
            case ConfigKey::PatienceMultiplier:
            case ConfigKey::CustomerSpawnMultiplier:
            case ConfigKey::NumStoreSpawns:
            case ConfigKey::UnlockedToilet:
            case ConfigKey::PissTimer:
            case ConfigKey::BladderSize:
            case ConfigKey::HasCityMultiplier:
            case ConfigKey::DrinkCostMultiplier:
            case ConfigKey::VomitFreqMultiplier:
            case ConfigKey::VomitAmountMultiplier:
            case ConfigKey::DayCount:
            case ConfigKey::Entity:
            case ConfigKey::CustomerSpawn:
                break;
        }
    });
}

}  // namespace progression
}  // namespace system_manager
