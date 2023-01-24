
#pragma once

#include "base_component.h"

struct CanBeTakenFrom : public BaseComponent {
    virtual ~CanBeTakenFrom() {}

    [[nodiscard]] bool can_take_from() const { return allowed; }
    [[nodiscard]] bool cannot_take_from() const { return !can_take_from(); }

    void update(bool g) { allowed = g; }

   private:
    bool allowed = false;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.value1b(allowed);
    }
};
