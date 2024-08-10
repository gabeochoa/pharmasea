
#include <algorithm>
#include <random>

#include "../building_locations.h"
#include "../components/has_day_night_timer.h"
#include "../components/is_progression_manager.h"
#include "../components/is_round_settings_manager.h"
#include "../dataclass/upgrades.h"
#include "../entity_helper.h"
#include "../entity_query.h"
#include "magic_enum/magic_enum.hpp"
#include "system_manager.h"

namespace system_manager {
namespace progression {

inline void skip_upgrade_visit() {
    GameState::get().transition_to_game();
    SystemManager::get().for_each_old([](Entity& e) {
        if (check_type(e, EntityType::Player)) {
            move_player_SERVER_ONLY(e, game::State::InGame);
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
    // std::shuffle(std::begin(options), std::end(options),
    // RandomEngine()::generator());

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

    std::vector<std::shared_ptr<UpgradeImpl>> possible_upgrades =
        irsm.config.get_possible_upgrades(ipm);

    log_info("num upgrades without filters : {}",
             magic_enum::enum_count<UpgradeClass>());

    // possible_upgrades.push_back(make_upgrade(UpgradeClass::CantEvenTell));
    // possible_upgrades.push_back(make_upgrade(UpgradeClass::UnlockToilet));

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

    ipm.upgradeOption1 = possible_upgrades[0]->type;
    ipm.upgradeOption2 = possible_upgrades[1]->type;

    log_info(" The two options we got are {} {} and {} {}",
             possible_upgrades[0]->name.debug(),
             magic_enum::enum_name<UpgradeClass>(possible_upgrades[0]->type),
             possible_upgrades[1]->name.debug(),
             magic_enum::enum_name<UpgradeClass>(possible_upgrades[1]->type));
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

    // unlock doors
    for (RefEntity door : EntityQuery()
                              .whereType(EntityType::Door)
                              .whereInside(PROGRESSION_BUILDING.min(),
                                           PROGRESSION_BUILDING.max())
                              .gen()) {
        door.get().removeComponentIfExists<IsSolid>();
    }
}

inline void update_upgrade_variables() {
    // For certain config keys, we dont "read" them live so to speak
    // and so we might need to update manually when they change

    Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
    IsRoundSettingsManager& irsm = sophie.get<IsRoundSettingsManager>();

    magic_enum::enum_for_each<ConfigKey>([&](auto val) {
        constexpr ConfigKey key = val;

        switch (key) {
            case ConfigKey::RoundLength: {
                // TODO right now we are just making the night longer
                // but maybe we want the day to be longer too?
                HasDayNightTimer& hasTimer = sophie.get<HasDayNightTimer>();
                hasTimer.set_night_length(
                    irsm.get<float>(ConfigKey::RoundLength));

            } break;
            case ConfigKey::Test:
            case ConfigKey::StoreRerollPrice:
            case ConfigKey::MaxNumOrders:
            case ConfigKey::PatienceMultiplier:
            case ConfigKey::CustomerSpawnMultiplier:
            case ConfigKey::NumStoreSpawns:
            case ConfigKey::PissTimer:
            case ConfigKey::BladderSize:
            case ConfigKey::DrinkCostMultiplier:
            case ConfigKey::VomitFreqMultiplier:
            case ConfigKey::VomitAmountMultiplier:
            case ConfigKey::MaxDrinkTime:
                break;
        }
    });
}

}  // namespace progression
}  // namespace system_manager
