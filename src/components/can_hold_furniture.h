

#pragma once

#include "../entity_helper.h"
#include "../entity_id.h"
#include "../vendor_include.h"
#include "base_component.h"

using EntityID = int;

struct CanHoldFurniture : public BaseComponent {
    virtual ~CanHoldFurniture() {}

    [[nodiscard]] bool empty() const {
        return held_furniture_handle.is_invalid();
    }
    [[nodiscard]] bool is_holding_furniture() const { return !empty(); }

    void update(EntityHandle handle, vec3 pickup_location) {
        held_furniture_handle = handle;
        pos = pickup_location;
    }

    [[nodiscard]] EntityHandle furniture_handle() const {
        return held_furniture_handle;
    }
    // Legacy compatibility
    [[nodiscard]] EntityID furniture_id() const {
        auto opt = afterhours::EntityHelper::resolve(held_furniture_handle);
        return opt ? opt->id : entity_id::INVALID;
    }
    [[nodiscard]] vec3 picked_up_at() const { return pos; }

   private:
    EntityHandle held_furniture_handle = EntityHandle::invalid();
    vec3 pos;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.object(held_furniture_handle);
        s.object(pos);
    }
};
