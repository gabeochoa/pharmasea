

#pragma once

#include "../engine/bitset_utils.h"
//
#include "../dataclass/ingredient.h"
#include "../recipe_library.h"
#include "base_component.h"

struct IsProgressionManager : public BaseComponent {
    virtual ~IsProgressionManager() {}

    IsProgressionManager() {}

    void init() {
        unlock_drink(Drink::coke);
        unlock_ingredient(Ingredient::Soda);

        log_trace("create: {} {}", enabledDrinks, enabledIngredients);
    }

    IsProgressionManager& unlock_drink(const Drink& drink) {
        log_trace("unlocking drink {}", magic_enum::enum_name<Drink>(drink));
        enabledDrinks.set(drink);
        log_trace("unlock drink: {} {}", enabledDrinks, enabledIngredients);
        return *this;
    }

    IsProgressionManager& unlock_ingredient(const Ingredient& ing) {
        log_trace("unlocking ingredient {}",
                  magic_enum::enum_name<Ingredient>(ing));
        enabledIngredients.set(ing);
        log_trace("unlock ingredient: {} {}", enabledDrinks,
                  enabledIngredients);
        return *this;
    }

    [[nodiscard]] DrinkSet enabled_drinks() const { return enabledDrinks; }
    [[nodiscard]] IngredientBitSet enabled_ingredients() const {
        return enabledIngredients;
    }

    Drink get_random_drink() const {
        log_trace("get random: {} {}", enabledDrinks, enabledIngredients);
        int drinkSetBit = bitset_utils::get_random_enabled_bit(enabledDrinks);
        if (drinkSetBit == -1) {
            log_warn("generated {} but we had {} enabled drinks", drinkSetBit,
                     enabledDrinks.count());
            return Drink::coke;
        }
        return magic_enum::enum_cast<Drink>(drinkSetBit).value();
    }

    // checks the current enabled ingredients to see if its possible to
    // create the drink
    bool can_create_drink(Drink drink) const {
        IngredientBitSet ings = get_recipe_for_drink(drink);

        IngredientBitSet overlap = ings & enabledIngredients;
        return overlap == ings;
    }

    [[nodiscard]] bool is_drink_unlocked(Drink drink) const {
        size_t index = magic_enum::enum_index<Drink>(drink).value();
        return enabledDrinks.test(index);
    }

    [[nodiscard]] bool is_ingredient_unlocked(Ingredient ingredient) const {
        size_t index = magic_enum::enum_index<Ingredient>(ingredient).value();
        return enabledIngredients.test(index);
    }

    [[nodiscard]] bool is_ingredient_locked(Ingredient ingredient) const {
        return !is_ingredient_unlocked(ingredient);
    }

    // TODO make private
    bool isUpgradeRound = true;
    bool collectedOptions = false;
    Drink option1 = coke;
    Drink option2 = coke;

    DrinkSet enabledDrinks;
    IngredientBitSet enabledIngredients;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.ext(enabledDrinks, bitsery::ext::StdBitset{});
        s.ext(enabledIngredients, bitsery::ext::StdBitset{});
    }
};
