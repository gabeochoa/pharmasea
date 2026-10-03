
#pragma once

#include <iostream>

#include "../vendor_include.h"
#include "base_component.h"
#include "transform.h"

struct CustomHeldItemPosition : public BaseComponent {
    enum struct Positioner {
        Table,
        Conveyer,
        ItemHoldingItem,
        PnumaticPipe,
        Blender,
    } positioner = Positioner::Table;

    void init(Positioner p) { positioner = p; }

   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        (void) self;
        return archive(                        //
            static_cast<BaseComponent&>(self)  //
        );
        // The reason we dont need to serialize this is because
        // the position is literally the actual position of the item
        // which is already serialized.
        //
        // if we decide to make a visual / real position difference
        // then itll have to be
        // s.value4b(positioner);
    }
};

inline std::ostream& operator<<(std::ostream& os,
                                const CustomHeldItemPosition::Positioner& p) {
    os << magic_enum::enum_name(p);
    return os;
}
