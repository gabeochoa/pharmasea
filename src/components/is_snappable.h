
#pragma once

#include "base_component.h"

struct IsSnappable : public BaseComponent {
    virtual ~IsSnappable() {}

   private:
    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseComponent>(this));
    }
};

CEREAL_REGISTER_TYPE(IsSnappable);
