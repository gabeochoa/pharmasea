

#pragma once

#include "bitsery/ext/std_bitset.h"
//
#include "../dataclass/ingredient.h"
#include "base_component.h"

struct CanOrderDrink : public BaseComponent {
    virtual ~CanOrderDrink() {}

    [[nodiscard]] Drink order() const { return my_order; }
    [[nodiscard]] bool has_order() const { return order().any(); }
    void update(Drink new_order) { my_order = new_order; }

   private:
    Drink my_order;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.ext(my_order, bitsery::ext::StdBitset{});
    }
};
