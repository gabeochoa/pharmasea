

#pragma once

#include "../ah.h"
#include "../dataclass/ingredient.h"
#include "../engine/random_engine.h"
#include "../recipe_library.h"
#include "base_component.h"

struct CanOrderDrink : public BaseComponent {
    enum OrderState {
        NeedsReset,    // i dont know what i want
        Ordering,      // i should go wait in line
        DrinkingNow,   // im drinking
        DoneDrinking,  // I dont want any more drinks im going home
    } order_state;

    CanOrderDrink()
        : num_orders_rem(-1), num_orders_had(0), current_order(Drink::coke) {
        order_state = OrderState::NeedsReset;
    }

    [[nodiscard]] IngredientBitSet recipe() const {
        return get_recipe_for_drink(current_order);
    }

    [[nodiscard]] Drink order() const { return current_order; }

    [[nodiscard]] bool current_order_has_alcohol() const {
        bool yes = false;
        bitset_utils::for_each_enabled_bit(
            recipe(), [&](size_t index) -> bitset_utils::ForEachFlow {
                if (yes) return bitset_utils::ForEachFlow::Break;
                Ingredient ig = magic_enum::enum_value<Ingredient>(index);
                yes |= ingredient::is_alcohol(ig);
                return bitset_utils::ForEachFlow::NormalFlow;
            });
        return yes;
    }

    [[nodiscard]] bool has_order() const {
        return order_state == OrderState::Ordering;
    }

    [[nodiscard]] std::string icon_name() const {
        if (order_state == OrderState::DrinkingNow) return "jug";
        return get_icon_name_for_drink(current_order);
    }

    void on_order_finished() {
        num_orders_rem--;
        num_orders_had++;
        drinks_in_bladder++;

        if (current_order_has_alcohol()) {
            num_alcoholic_drinks_had++;
        }
        order_state = num_orders_rem > 0
                          ? CanOrderDrink::OrderState::Ordering
                          : CanOrderDrink::OrderState::DoneDrinking;
    }

    auto& set_first_order(Drink d) {
        forced_first_order = d;
        return *this;
    }

    void empty_bladder() { drinks_in_bladder = 0; }

    void set_order(Drink new_order) { current_order = new_order; }
    [[nodiscard]] Drink get_order() const { return current_order; }

    void increment_tab(int amount) { tab_cost += amount; }
    void increment_tip(int amount) { tip += amount; }

    void apply_tip_multiplier(float mult) {
        tip = static_cast<int>(floor(tip * mult));
    }

    void reset_customer(int max_num_orders, Drink randomNextDrink) {
        num_orders_rem = RandomEngine::get().get_int(1, max_num_orders);

        num_orders_had = 0;
        // If we have a forced order use that otherwise grab a random unlocked
        // drink
        Drink next_drink_order = forced_first_order.value_or(randomNextDrink);
        current_order = next_drink_order;
        order_state = CanOrderDrink::OrderState::Ordering;

        forced_first_order = {};
    }

    [[nodiscard]] bool wants_more_drinks() const { return num_orders_rem > 0; }
    [[nodiscard]] int get_drinks_in_bladder() const {
        return drinks_in_bladder;
    }

    [[nodiscard]] int get_current_tab() { return tab_cost; }
    [[nodiscard]] int get_current_tip() { return tip; }

    void clear_tab_and_tip() {
        tab_cost = 0;
        tip = 0;
    }

    [[nodiscard]] int num_drinks_drank() const { return num_orders_had; }
    [[nodiscard]] int num_alcoholic_drinks_drank() const {
        return num_alcoholic_drinks_had;
    }

   private:
    int num_orders_rem = -1;
    int num_orders_had = 0;
    Drink current_order;

    std::optional<Drink> forced_first_order;

    int num_alcoholic_drinks_had = 0;

    int tab_cost = 0;
    int tip = 0;
    int drinks_in_bladder = 0;

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                         //
            static_cast<BaseComponent&>(self),  //
            self.current_order,                 //
            self.order_state                    //
        );
    }
};
