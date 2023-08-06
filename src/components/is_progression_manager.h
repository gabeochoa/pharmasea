

#pragma once

#include "../engine/bitset_utils.h"
//
#include "../dataclass/ingredient.h"
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

   private:
    DrinkSet enabledDrinks;
    IngredientBitSet enabledIngredients;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
    }
};
