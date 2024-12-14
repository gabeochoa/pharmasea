

#pragma once

#include "base_component.h"

struct CanBeHeld : public BaseComponent {
    virtual ~CanBeHeld() {}

    [[nodiscard]] bool is_held() const { return held; }
    [[nodiscard]] bool is_not_held() const { return !is_held(); }

    void set_is_being_held(bool g) { held = g; }

   private:
    bool held = false;

    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseComponent>(this), held);
    }
};

struct CanBeHeld_HT : public CanBeHeld {};

CEREAL_REGISTER_TYPE(CanBeHeld);
CEREAL_REGISTER_TYPE(CanBeHeld_HT);
