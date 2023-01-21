
#pragma once

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

}  // namespace system_manager

struct SystemManager {
    void always_update(float dt) {
        EntityHelper::forEachEntity([dt](std::shared_ptr<Entity> entity) {
            system_manager::transform_snapper(entity, dt);
            return EntityHelper::ForEachFlow::None;
        });
    }
};
