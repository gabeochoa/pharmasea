

#pragma once

#include "base_component.h"

struct IsSolid : public BaseComponent {
    virtual ~IsSolid() {}

   private:
    friend class cereal::access;
    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseComponent>(this));
    }
};
CEREAL_REGISTER_TYPE(IsSolid);
