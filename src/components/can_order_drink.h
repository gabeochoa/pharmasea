

#pragma once

#include "bitsery/ext/std_bitset.h"
//
#include "../dataclass/order.h"
#include "base_component.h"

struct CanOrderDrink : public BaseComponent {
    virtual ~CanOrderDrink() {}

    [[nodiscard]] Order order() const { return my_order; }
    [[nodiscard]] bool has_order() const { return order().is_active(); }
    void update(Order new_order) { my_order = new_order; }

   private:
    Order my_order;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.object(my_order);
    }
};
