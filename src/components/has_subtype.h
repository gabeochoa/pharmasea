

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
    HasSubtype() : start(INVALID), end(INVALID), type_index(INVALID) {}
    HasSubtype(int st_start, int st_end, int st_type = -1)
        : start(st_start), end(st_end), type_index(st_type) {
        if (type_index == -1) type_index = get_random_index();
    }
    virtual ~HasSubtype() {}

    [[nodiscard]] int get_num_types() const { return end - start; }
    [[nodiscard]] int get_random_index() const {
        int index = randIn(0, get_num_types());
        return start + index;
    }
    [[nodiscard]] Subtype get_type() const {
        return magic_enum::enum_cast<Subtype>(type_index).value();
    }

    [[nodiscard]] int get_type_index() const { return type_index; }

    void increment_type() {
        int index = type_index;
        if (index == end) index = (start - 1);
        type_index = index + 1;
    }

    template<typename T>
    [[nodiscard]] std::string_view as_type() const {
        // TODO Why do we need the plus 1 here?
        T t = magic_enum::enum_value<T>(start + get_type_index());
        return magic_enum::enum_name<T>(t);
    }

   private:
    int start;
    int end;
    int type_index;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value4b(start);
        s.value4b(end);
        s.value4b(type_index);
    }
};
