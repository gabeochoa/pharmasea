

#pragma once

#include "bitsery/ext/std_bitset.h"
//
#include "../dataclass/ingredient.h"
#include "../engine/bitset_utils.h"
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

    [[nodiscard]] bool current_order_has_alcohol() const {
        bool yes = false;
        bitset_utils::for_each_enabled_bit(recipe(), [&](size_t index) {
            // TODO add support for flow control
            if (yes) return;
            Ingredient ig = magic_enum::enum_value<Ingredient>(index);
            yes |= array_contains(ingredient::Alcohols, ig);
        });
        return yes;
    }

    [[nodiscard]] bool has_order() const {
        return order_state == OrderState::Ordering;
    }

    [[nodiscard]] const std::string icon_name() const {
        // TODO this doesnt work today
        if (order_state == OrderState::DrinkingNow) return "jug";
        return get_icon_name_for_drink(current_order);
    }

    void on_order_finished() {
        num_orders_rem--;
        num_orders_had++;

        if (current_order_has_alcohol()) {
            num_alcoholic_drinks_had++;
        }
    }

    // TODO make private
    int num_orders_rem = -1;
    int num_orders_had = 0;
    Drink current_order;

    int num_alcoholic_drinks_had = 0;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value4b(current_order);
    }
};
