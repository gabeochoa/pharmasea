

#pragma once

#include "bitsery/ext/std_bitset.h"
//
#include "../dataclass/ingredient.h"
#include "../engine/random.h"
#include "../recipe_library.h"
#include "base_component.h"

struct CanOrderDrink : public BaseComponent {
    enum OrderState {
        NeedsReset,
        Ordering,
        DrinkingNow,
        DoneDrinking,
    } order_state;

    CanOrderDrink()
        : num_orders_rem(-1), num_orders_had(0), current_order(Drink::coke) {
        order_state = OrderState::NeedsReset;
    }

    virtual ~CanOrderDrink() {}

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
    int num_orders_had = 0;
    Drink current_order;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value4b(current_order);
    }
};
