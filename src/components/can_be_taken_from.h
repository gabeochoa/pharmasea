
#pragma once

#include "base_component.h"

struct CanBeTakenFrom : public BaseComponent {
    virtual ~CanBeTakenFrom() {}

    [[nodiscard]] bool can_take_from() const { return allowed; }
    [[nodiscard]] bool cannot_take_from() const { return !can_take_from(); }

    void update(bool g) { allowed = g; }

   private:
    bool allowed = false;

    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseComponent>(this), allowed);
    }
};

CEREAL_REGISTER_TYPE(CanBeTakenFrom);
