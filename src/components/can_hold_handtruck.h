

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

    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseComponent>(this), held_hand_truck_id,
                pos);
    }
};

CEREAL_REGISTER_TYPE(CanHoldHandTruck);
