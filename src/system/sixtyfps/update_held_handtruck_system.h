#pragma once

#include "../../ah.h"
#include "../../components/can_hold_handtruck.h"
#include "../../components/transform.h"
#include "../../entity_helper.h"

namespace system_manager {

struct UpdateHeldHandTruckPositionSystem
    : public afterhours::System<CanHoldHandTruck, Transform> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity&, CanHoldHandTruck& can_hold_hand_truck,
                               Transform& transform, float) override {
        if (can_hold_hand_truck.empty()) return;

        auto new_pos = transform.pos();

        OptEntity hand_truck =
            EntityHelper::getEntityForID(can_hold_hand_truck.held_id());
        hand_truck->get<Transform>().update(new_pos);
    }
};

}  // namespace system_manager
