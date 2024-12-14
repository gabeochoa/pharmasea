

#pragma once

#include "../vendor_include.h"
#include "base_component.h"

using EntityID = int;

struct CanHoldHandTruck : public BaseComponent {
    virtual ~CanHoldHandTruck() {}

    [[nodiscard]] bool empty() const { return held_hand_truck_id == -1; }
    [[nodiscard]] bool is_holding() const { return !empty(); }

    void update(EntityID hand_truck_id, vec3 pickup_location) {
        held_hand_truck_id = hand_truck_id;
        pos = pickup_location;
    }

    [[nodiscard]] EntityID hand_truck_id() const { return held_hand_truck_id; }
    [[nodiscard]] vec3 picked_up_at() const { return pos; }

   private:
    int held_hand_truck_id = -1;
    vec3 pos;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value4b(held_hand_truck_id);
        s.object(pos);
    }
};
