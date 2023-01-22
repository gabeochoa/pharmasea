
#pragma once

#include "../item.h"
#include "base_component.h"
#include "transform.h"

struct CustomHeldItemPosition : public BaseComponent {
    enum struct Positioner {
        Default,
        Table,
        Conveyer
    } positioner = Positioner::Default;

    void init(Positioner p) { positioner = p; }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.value4b(positioner);
    }
};
