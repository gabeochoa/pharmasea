

#pragma once

#include "base_component.h"

struct IsItem : public BaseComponent {
    enum HeldBy {
        NONE,
        UNKNOWN,
        ITEM,
        PLAYER,
        UNKNOWN_FURNITURE,
        CUSTOMER,
        BLENDER
    };

    virtual ~IsItem() {}

    void set_held_by(HeldBy hb) { held_by = hb; }

    [[nodiscard]] bool is_held_by(HeldBy hb = HeldBy::NONE) const {
        return held_by == hb;
    }

    [[nodiscard]] bool is_not_held_by(HeldBy hb) const {
        return !is_held_by(hb);
    }

   private:
    HeldBy held_by = NONE;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value4b(held_by);
    }
};
