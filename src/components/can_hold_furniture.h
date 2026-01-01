

#pragma once

#include "../vendor_include.h"
#include "base_component.h"

#include "../persistent_entity_ref.h"

struct CanHoldFurniture : public BaseComponent {
    [[nodiscard]] bool empty() const {
        return held_furniture.id == entity_id::INVALID;
    }
    [[nodiscard]] bool is_holding_furniture() const { return !empty(); }

    void update(EntityID furniture_id, vec3 pickup_location) {
        held_furniture.set_id(furniture_id);
        pos = pickup_location;
    }

    [[nodiscard]] EntityID furniture_id() const { return held_furniture.id; }
    [[nodiscard]] vec3 picked_up_at() const { return pos; }

   private:
    PersistentEntityRef held_furniture{};
    vec3 pos;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.object(held_furniture);
        s.object(pos);
    }
};
