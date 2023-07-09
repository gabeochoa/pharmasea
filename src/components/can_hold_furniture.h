

#pragma once

#include "base_component.h"

struct CanHoldFurniture : public BaseComponent {
    CanHoldFurniture() { load_held(); }
    virtual ~CanHoldFurniture() {}

    void update(Entity& furniture) {
        held_furniture = asOpt(furniture);
        held_entity_id = furniture.id;
    }
    void unown() { held_furniture = {}; }

    [[nodiscard]] bool empty() const { return !valid(held_furniture); }
    [[nodiscard]] bool is_holding_furniture() const { return !empty(); }

    [[nodiscard]] OptEntity furniture() const { return held_furniture; }

    void load_held() {
        if (valid(held_furniture)) return;
        // TODO add cpp for enitty helper
        // held_furniture = EntityHelper::findEntity(held_entity_id);
    }

   private:
    OptEntity held_furniture;
    int held_entity_id = -1;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.value4b(held_entity_id);
    }
};
