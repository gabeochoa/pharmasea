
#pragma once

#include "base_component.h"

using EntityID = int;

struct HasLastInteractedCustomer : public BaseComponent {
    EntityID customer_id = -1;

    virtual ~HasLastInteractedCustomer() {}

   private:
    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseComponent>(this),
                //
                customer_id);
    }
};

CEREAL_REGISTER_TYPE(HasLastInteractedCustomer);
