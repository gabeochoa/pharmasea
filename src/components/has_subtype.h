

#pragma once

#include "../engine/random.h"
#include "../vendor_include.h"
#include "base_component.h"

enum Subtype {
    INVALID = 0,

    PILL_START,
    PillRed,
    PillRedLong,
    PillBlue,
    PillBlueLong,
    PILL_END,

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
