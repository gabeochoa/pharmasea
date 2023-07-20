

#pragma once

#include "../vendor_include.h"
//
#include "../engine/random.h"
#include "ingredient.h"

struct Order {
    Order() {
        num_orders_rem = randIn(0, 5);
        current_order = get_random_drink();

        timeBetweenDrinks = randfIn(1.f, 5.f);
    }

    [[nodiscard]] bool is_active() const { return current_order.any(); }

    void pass_time(float dt) {
        // Dont pass time if we have a drink order
        if (!is_active()) return;

        // We got our drink, we'll be back soon

        timeSinceLastDrink += dt;
        if (timeSinceLastDrink >= timeBetweenDrinks) {
            timeSinceLastDrink = 0;
            num_orders_rem--;
            current_order = get_random_drink();
        }
    }

    int num_orders_rem = -1;
    Drink current_order;

    float timeBetweenDrinks = 1.f;
    float timeSinceLastDrink = 1.f;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(current_order, bitsery::ext::StdBitset{});
        s.value4b(num_orders_rem);
        s.value4b(timeBetweenDrinks);
        s.value4b(timeSinceLastDrink);
    }
};
