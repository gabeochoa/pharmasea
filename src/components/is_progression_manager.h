

#pragma once

#include "../engine/bitset_utils.h"
//
#include "../dataclass/ingredient.h"
#include "../recipe_library.h"
#include "base_component.h"

struct IsProgressionManager : public BaseComponent {
    virtual ~IsProgressionManager() {}

    IsProgressionManager() {
        // TODO need to decide if we unblock ingredients and that enables drinks
        // or if we unlock drinks and that enables ingredients
        enabledDrinks |= Drink::coke;
        enabledDrinks |= Drink::rum_and_coke;

        enabledIngredients |= Ingredient::Soda;
        enabledIngredients |= Ingredient::Rum;

        // TODO add progression / level logic
        // enabledDrinks.set();
    }

    auto& unlock_drink(Drink& drink) {
        enabledDrinks |= drink;
        return *this;
    }

    auto& unlock_ingredient(Ingredient& ing) {
        enabledIngredients |= ing;
        return *this;
    }

    [[nodiscard]] DrinkSet enabled_drinks() const { return enabledDrinks; }
    [[nodiscard]] IngredientBitSet enabled_ingredients() const {
        return enabledIngredients;
    }

    Drink get_random_drink() const {
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

    // TODO rename function to be more clear
    bool drink_unlocked(Drink drink) const { return enabledDrinks.test(drink); }

    // TODO make private
    bool isUpgradeRound = true;
    bool collectedOptions = false;
    Drink option1;
    Drink option2;

    DrinkSet enabledDrinks;
    IngredientBitSet enabledIngredients;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
    }
};
