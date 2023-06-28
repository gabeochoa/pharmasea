

#pragma once

#include "base_component.h"

struct CanBeHeld : public BaseComponent {
    virtual ~CanBeHeld() {}

    [[nodiscard]] bool is_held() const { return held; }
    [[nodiscard]] bool is_not_held() const { return !is_held(); }

    void update(bool g) { held = g; }

   private:
    bool held = false;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value1b(held);
    }
};
