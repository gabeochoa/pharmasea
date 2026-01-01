
#pragma once

#include "base_component.h"

#include "../persistent_entity_ref.h"

struct HasLastInteractedCustomer : public BaseComponent {
    PersistentEntityRef customer{};

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.object(customer);
    }
};
