

#pragma once

#include "../vendor_include.h"
//
#include "../engine/random.h"
#include "base_component.h"

enum Subtype {
    INVALID = -1,
    ST_NONE = 0,  // If you are getting none, you probably have to increment
                  // your index by ITEM_START

    PILL_START = 1,
    PillRed = 1,
    PillRedLong = 2,
    PillBlue = 3,
    PillBlueLong = 4,
    PILL_END = 4
};

struct HasSubtype : public BaseComponent {
    HasSubtype() : start(INVALID), end(INVALID), type(INVALID) {}
    HasSubtype(int st_start, int st_end, Subtype st_type = INVALID)
        : start(st_start), end(st_end), type(st_type) {
        if (type == INVALID) type = get_random_type();
    }
    virtual ~HasSubtype() {}

    [[nodiscard]] int get_num_types() const { return end - start; }
    [[nodiscard]] Subtype get_random_type() const {
        int index = randIn(0, get_num_types());
        return magic_enum::enum_value<Subtype>(start + index);
    }
    [[nodiscard]] Subtype get_type() const { return type; }

    void increment_type() {
        int index = magic_enum::enum_integer<Subtype>(type);
        if (index == end) index = (start - 1);
        type = magic_enum::enum_cast<Subtype>(index + 1).value();
    }

   private:
    int start;
    int end;
    Subtype type;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value4b(start);
        s.value4b(end);
        s.value4b(type);
    }
};
