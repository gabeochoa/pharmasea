
#pragma once

#include "../components/custom_item_position.h"
#include "../components/transform.h"
#include "../entity.h"
#include "../entityhelper.h"

namespace system_manager {

inline void transform_snapper(std::shared_ptr<Entity> entity, float) {
    if (!entity->has<Transform>()) return;

    Transform& transform = entity->get<Transform>();
    if (entity->is_snappable()) {
        transform.position = transform.snap_position();
    } else {
        transform.position = transform.raw_position;
    }
}

inline void update_held_item_position(std::shared_ptr<Entity> entity, float) {
    if (!entity->has<CanHoldItem>()) return;
    CanHoldItem& can_hold_item = entity->get<CanHoldItem>();
    if (can_hold_item.empty()) return;

    if (!entity->has<Transform>()) return;
    Transform& transform = entity->get<Transform>();

    if (entity->has<CustomHeldItemPosition>()) {
        CustomHeldItemPosition& custom_item_position =
            entity->get<CustomHeldItemPosition>();

        if (custom_item_position.mutator) {
            can_hold_item.item()->update_position(
                custom_item_position.mutator(transform));
        } else {
            log_warn(
                "Entity has custom held item position but didnt initalize the "
                "component");
        }
        return;
    }

    // Default

    auto new_pos = transform.position;
    if (transform.face_direction & Transform::FrontFaceDirection::FORWARD) {
        new_pos.z += TILESIZE;
    }
    if (transform.face_direction & Transform::FrontFaceDirection::RIGHT) {
        new_pos.x += TILESIZE;
    }
    if (transform.face_direction & Transform::FrontFaceDirection::BACK) {
        new_pos.z -= TILESIZE;
    }
    if (transform.face_direction & Transform::FrontFaceDirection::LEFT) {
        new_pos.x -= TILESIZE;
    }
    can_hold_item.item()->update_position(new_pos);
}

}  // namespace system_manager

struct SystemManager {
    void always_update(float dt) {
        EntityHelper::forEachEntity([dt](std::shared_ptr<Entity> entity) {
            system_manager::transform_snapper(entity, dt);
            system_manager::update_held_item_position(entity, dt);
            return EntityHelper::ForEachFlow::None;
        });
    }
};
