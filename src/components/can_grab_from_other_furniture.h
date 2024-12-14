

#pragma once

#include "base_component.h"

struct CanGrabFromOtherFurniture : public BaseComponent {
    virtual ~CanGrabFromOtherFurniture() {}

   private:
    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseComponent>(this));
    }
};

CEREAL_REGISTER_TYPE(CanGrabFromOtherFurniture);
