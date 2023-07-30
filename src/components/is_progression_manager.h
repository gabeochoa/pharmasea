

#pragma once

#include "../engine/bitset_utils.h"
//
#include "../dataclass/ingredient.h"
#include "base_component.h"

struct IsProgressionManager : public BaseComponent {
    virtual ~IsProgressionManager() {}

    IsProgressionManager() {
        enabledDrinks |= Drink::coke;
        enabledDrinks |= Drink::rum_and_coke;
        // TODO add progression / level logic
        // enabledDrinks.set();
    }

    [[nodiscard]] DrinkSet enabled_drinks() const { return enabledDrinks; }

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

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
    }
};
