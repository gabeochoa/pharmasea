

#pragma once

#include <bitset>

#include "../dataclass/ingredient.h"
#include "base_component.h"

// TODO make a bitset utils

template<typename T>
int index_of_nth_set_bit(const T& bitset, int n) {
    int count = 0;
    for (size_t i = 0; i < bitset.size(); ++i) {
        if (bitset.test(i)) {
            if (++count == n) {
                return static_cast<int>(i);
            }
        }
    }
    return -1;  // Return -1 if the nth set bit is not found
}

struct IsProgressionManager : public BaseComponent {
    virtual ~IsProgressionManager() {}

    IsProgressionManager() {
        enabledDrinks |= Drink::coke;
        enabledDrinks |= Drink::rum_and_coke;
    }

    [[nodiscard]] DrinkSet enabled_drinks() const { return enabledDrinks; }

    Drink get_random_drink() const {
        size_t num_drinks = enabledDrinks.count();
        int randIndx = randIn(0, (int) num_drinks);
        int drinkSetIndex =
            index_of_nth_set_bit<DrinkSet>(enabledDrinks, randIndx);
        log_info("num drinks {} rand index {} drink index {}", num_drinks,
                 randIndx, drinkSetIndex);
        // TODO unlikely but handle an error and default to coke
        return magic_enum::enum_cast<Drink>(drinkSetIndex).value();
    }

   private:
    DrinkSet enabledDrinks;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
    }
};
