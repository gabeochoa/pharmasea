

#pragma once

#include "../vendor_include.h"
//
#include "../engine/random.h"
#include "ingredient.h"

struct Order {
    Order() {
        num_orders_rem = randIn(0, 5);
        current_order = get_random_drink();
        recipe = get_recipe_for_drink(current_order);

        timeBetweenDrinks = randfIn(1.f, 5.f);
        timeSinceLastDrink = timeBetweenDrinks;
    }

    [[nodiscard]] bool is_active() const { return recipe.any(); }
    [[nodiscard]] Drink drink() const { return current_order; }

    void pass_time(float dt) {
        // Dont pass time if we have a drink order
        if (!is_active()) return;

        // We got our drink, we'll be back soon

        timeSinceLastDrink += dt;
        if (timeSinceLastDrink >= timeBetweenDrinks) {
            timeSinceLastDrink = 0;
            num_orders_rem--;
            current_order = get_random_drink();
            recipe = get_recipe_for_drink(current_order);
        }
    }

    int num_orders_rem = -1;
    Drink current_order;
    IngredientBitSet recipe;

    float timeBetweenDrinks = 1.f;
    float timeSinceLastDrink = 1.f;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(recipe, bitsery::ext::StdBitset{});
        s.value4b(current_order);
        s.value4b(num_orders_rem);
        s.value4b(timeBetweenDrinks);
        s.value4b(timeSinceLastDrink);
    }
};
