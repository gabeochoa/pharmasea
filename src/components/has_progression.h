
#pragma once

#include "../entity_type.h"
#include "base_component.h"

struct HasProgression : public BaseComponent {
    virtual ~HasProgression() {}

   private:
    friend class cereal::access;
    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseComponent>(this));
    }
};

CEREAL_REGISTER_TYPE(HasProgression);
