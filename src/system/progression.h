
#include <random>

#include "../components/is_progression_manager.h"
#include "system_manager.h"

namespace system_manager {
namespace progression {

inline void collect_upgrade_options(Entity& entity, float dt) {
    if (entity.is_missing<IsProgressionManager>()) return;
    IsProgressionManager& ipm = entity.get<IsProgressionManager>();

    if (ipm.collectedOptions) {
        // we already got the options and cached them...
        return;
    }

    // If we arent in an upgrade round just go directly to planning
    if (!ipm.isUpgradeRound) {
        // TODO right now just do every other, but itll likely be less often
        // since theres not that many drinks, maybe every 5th round?
        ipm.isUpgradeRound = !ipm.isUpgradeRound;
        GameState::get().set(game::State::Planning);
        log_info("not an upgrade round see ya");
        return;
    }

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
            if (ipm.drink_unlocked(drink)) continue;

            IngredientBitSet ing = get_recipe_for_drink(drink);

            // TODO put these in utils somewhere so we can use when checking
            // overlap later
            IngredientBitSet ing_overlap = (ing & ipm.enabled_ingredients());
            IngredientBitSet ing_needed = ing ^ (ing_overlap);
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
        log_warn("no options, figure out what to do in this case");
        return;
    }

    log_info("possible options");
    for (auto d_o : options) {
        log_info("{} {} : {}", d_o.d, magic_enum::enum_name<Drink>(d_o.d),
                 d_o.num_ing_needed);
    }

    ipm.option1 = options[0].d;
    ipm.option2 = options[1].d;

    log_info("got options for progression {} and {}", ipm.option1, ipm.option2);

    ipm.collectedOptions = true;
}

}  // namespace progression
}  // namespace system_manager
