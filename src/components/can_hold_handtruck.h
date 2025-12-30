

#pragma once

#include "../entity_helper.h"
#include "../entity_id.h"
#include "../vendor_include.h"
#include "base_component.h"

using EntityID = int;

struct CanHoldHandTruck : public BaseComponent {
    virtual ~CanHoldHandTruck() {}

    [[nodiscard]] bool empty() const {
        if (held_hand_truck_handle.is_invalid()) return true;
        return !EntityHelper::resolve(held_hand_truck_handle).has_value();
    }
    [[nodiscard]] bool is_holding() const { return !empty(); }

    void update(EntityID hand_truck_id, vec3 pickup_location) {
        held_hand_truck_handle = EntityHelper::getHandleForID(hand_truck_id);
        pos = pickup_location;
    }

    void update(EntityHandle handle, vec3 pickup_location) {
        held_hand_truck_handle = handle;
        pos = pickup_location;
    }

    [[nodiscard]] EntityHandle hand_truck_handle() const {
        return held_hand_truck_handle;
    }
    // Legacy compatibility
    [[nodiscard]] EntityID hand_truck_id() const {
        auto opt = EntityHelper::resolve(held_hand_truck_handle);
        return opt ? opt->id : entity_id::INVALID;
    }
    [[nodiscard]] vec3 picked_up_at() const { return pos; }

   private:
    EntityHandle held_hand_truck_handle = EntityHandle::invalid();
    vec3 pos;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.object(held_hand_truck_handle);
        s.object(pos);
    }
};
