
#pragma once

#include "base_component.h"

#include "../entity_ref.h"

struct HasLastInteractedCustomer : public BaseComponent {
    EntityRef customer{};

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.object(customer);
    }
};
