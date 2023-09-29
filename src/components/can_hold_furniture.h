

#pragma once

#include "base_component.h"

using EntityID = int;

struct CanHoldFurniture : public BaseComponent {
    virtual ~CanHoldFurniture() {}

    [[nodiscard]] bool empty() const { return held_furniture_id == -1; }
    [[nodiscard]] bool is_holding_furniture() const { return !empty(); }

    void update(EntityID furniture_id) { held_furniture_id = furniture_id; }

    [[nodiscard]] EntityID furniture_id() const { return held_furniture_id; }

   private:
    int held_furniture_id = -1;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value4b(held_furniture_id);
    }
};
