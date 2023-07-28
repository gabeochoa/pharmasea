

#pragma once

#include "bitsery/ext/std_bitset.h"
//
#include "../dataclass/ingredient.h"
#include "../engine/random.h"
#include "../recipe_library.h"
#include "base_component.h"

struct CanOrderDrink : public BaseComponent {
    enum OrderState {
        Ordering,
        DrinkingNow,
        DoneDrinking,
    } order_state;

    CanOrderDrink() { reset(); }

    virtual ~CanOrderDrink() {}

    void reset() {
        // TODO eventually read from game settings
        num_orders_rem = randIn(0, 5);
        current_order = get_random_drink();
    }

    [[nodiscard]] IngredientBitSet recipe() const {
        return get_recipe_for_drink(current_order);
    }

    [[nodiscard]] bool has_order() const {
        return order_state == OrderState::Ordering;
    }

    [[nodiscard]] const std::string icon_name() const {
        // TODO this doesnt work today
        if (order_state == OrderState::DrinkingNow) return "jug";
        return get_icon_name_for_drink(current_order);
    }

    // TODO make private
    int num_orders_rem = -1;
    Drink current_order;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value4b(current_order);
    }
};
