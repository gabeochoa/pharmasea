#pragma once

#include "../../ah.h"
#include "../../building_locations.h"
#include "../../components/can_order_drink.h"
#include "../../components/transform.h"
#include "../../entities/entity_helper.h"
#include "../../vec_util.h"

namespace system_manager {

struct DeleteCustomersWhenLeavingInroundSystem
    : public afterhours::System<CanOrderDrink, Transform,
                                afterhours::tags::All<EntityType::Customer>> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity& entity, CanOrderDrink&,
                               Transform& transform, float) override {
        if (vec::distance(transform.as2(), {GATHER_SPOT, GATHER_SPOT}) > 2.f)
            return;
        entity.cleanup = true;
    }
};

}  // namespace system_manager
