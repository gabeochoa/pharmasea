

#pragma once

#include "base_component.h"

struct IsItem : public BaseComponent {
    enum HeldBy {
        ALL = -1,
        NONE = 0,
        UNKNOWN = 1 << 0,
        ITEM = 1 << 1,
        PLAYER = 1 << 2,
        UNKNOWN_FURNITURE = 1 << 3,
        CUSTOMER = 1 << 4,
        BLENDER = 1 << 5,
        SODA_MACHINE = 1 << 6,

    };

    virtual ~IsItem() {}

    void set_held_by(HeldBy hb) { held_by = hb; }

    [[nodiscard]] bool is_held_by(HeldBy hb = HeldBy::NONE) const {
        return held_by == hb;
    }

    [[nodiscard]] bool is_not_held_by(HeldBy hb) const {
        return !is_held_by(hb);
    }

    [[nodiscard]] bool can_be_held_by(HeldBy hb) const {
        return hb_filter == ALL ? true : hb_filter & hb;
    }

    IsItem& set_hb_filter(HeldBy hb) {
        hb_filter = hb;
        return *this;
    }

    [[nodiscard]] bool is_held() const {
        // TODO might need to do something more sophisticated
        return held_by != HeldBy::NONE;
    }

   private:
    HeldBy held_by = NONE;
    HeldBy hb_filter = ALL;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value4b(held_by);
    }
};
