#pragma once

#include "../../ah.h"
#include "../../components/can_hold_item.h"
#include "../../components/custom_item_position.h"
#include "../../components/transform.h"
#include "../../entity_helper.h"
#include "afterhours_held_position_helpers.h"

namespace system_manager {

struct UpdateHeldItemPositionSystem
    : public afterhours::System<CanHoldItem, Transform> {
    virtual void for_each_with(Entity& entity, CanHoldItem& can_hold_item,
                               Transform&, float) override {
        if (can_hold_item.empty()) return;

        vec3 new_pos = entity.has<CustomHeldItemPosition>()
                           ? get_new_held_position_custom(entity)
                           : get_new_held_position_default(entity);

        OptEntity held_opt = can_hold_item.item();
        if (!held_opt) {
            can_hold_item.update(nullptr, entity.id);
            return;
        }
        held_opt.asE().get<Transform>().update(new_pos);
    }
};

}  // namespace system_manager
