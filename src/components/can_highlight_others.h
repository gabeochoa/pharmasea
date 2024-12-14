

#pragma once

#include "base_component.h"

struct CanHighlightOthers : public BaseComponent {
    virtual ~CanHighlightOthers() {}

    [[nodiscard]] float reach() const { return furniture_reach; }

   private:
    float furniture_reach = 1.80f;
    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseComponent>(this));
    }
};

CEREAL_REGISTER_TYPE(CanHighlightOthers);
