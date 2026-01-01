
#pragma once

#include "base_component.h"

using EntityID = int;

struct HasLastInteractedCustomer : public BaseComponent {
    EntityID customer_id = -1;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value4b(customer_id);
    }
};
