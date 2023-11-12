
#include <random>

#include "../components/is_progression_manager.h"
#include "system_manager.h"

namespace system_manager {
namespace progression {

inline void collect_upgrade_options(Entity& entity, float) {
    if (entity.is_missing<IsProgressionManager>()) return;
    IsProgressionManager& ipm = entity.get<IsProgressionManager>();

    if (ipm.collectedOptions) {
        // we already got the options and cached them...
        return;
    }

    const auto transition_to_store = []() {
        GameState::get().transition_to_store();
        SystemManager::get().for_each_old([](Entity& e) {
            if (check_type(e, EntityType::Player)) {
                move_player_SERVER_ONLY(e, game::State::Store);
                return;
            }
        });
    };

    // If we arent in an upgrade round just go directly to planning
    if (!ipm.isUpgradeRound) {
        // TODO right now just do every other, but itll likely be less often
        // since theres not that many drinks, maybe every 5th round?
        log_info("not an upgrade round see ya");
        ipm.isUpgradeRound = !ipm.isUpgradeRound;
        transition_to_store();
        return;
    }

    ipm.isUpgradeRound = !ipm.isUpgradeRound;

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
        ipm.isUpgradeRound = false;
        transition_to_store();
        return;
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

    ipm.collectedOptions = true;
}

}  // namespace progression
}  // namespace system_manager
