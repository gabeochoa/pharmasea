
#pragma once

#include "base_component.h"

struct IsFreeInStore : public BaseComponent {
    virtual ~IsFreeInStore() {}

   private:
    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseComponent>(this));
    }
};

CEREAL_REGISTER_TYPE(IsFreeInStore);
