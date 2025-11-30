#pragma once

#include "../ah.h"
#include "../components/can_hold_item.h"
#include "../components/custom_item_position.h"
#include "../components/transform.h"
#include "system_manager.h"

namespace system_manager {

// Forward declarations for functions defined in system_manager.cpp
vec3 get_new_held_position_custom(Entity& entity);
vec3 get_new_held_position_default(Entity& entity);

struct UpdateHeldItemPositionSystem
    : public afterhours::System<CanHoldItem, Transform> {
    virtual void for_each_with(Entity& entity, CanHoldItem& can_hold_item,
                               Transform&, float) override {
        if (can_hold_item.empty()) return;

        vec3 new_pos = entity.has<CustomHeldItemPosition>()
                           ? get_new_held_position_custom(entity)
                           : get_new_held_position_default(entity);

        can_hold_item.item().get<Transform>().update(new_pos);
    }
};

}  // namespace system_manager
