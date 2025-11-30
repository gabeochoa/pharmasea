#pragma once

#include "../ah.h"
#include "../components/can_hold_furniture.h"
#include "../components/has_day_night_timer.h"
#include "../components/transform.h"
#include "../engine/statemanager.h"
#include "../entity_helper.h"

namespace system_manager {

struct UpdateHeldFurniturePositionSystem
    : public afterhours::System<CanHoldFurniture, Transform> {
    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;

        Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
        const HasDayNightTimer& hastimer = sophie.get<HasDayNightTimer>();
        return hastimer.is_daytime();
    }

    virtual void for_each_with([[maybe_unused]] Entity& entity,
                               CanHoldFurniture& can_hold_furniture,
                               Transform& transform, float) override {
        if (can_hold_furniture.empty()) return;

        auto new_pos = transform.pos();
        if (transform.face_direction() &
            Transform::FrontFaceDirection::FORWARD) {
            new_pos.z += TILESIZE;
        }
        if (transform.face_direction() & Transform::FrontFaceDirection::RIGHT) {
            new_pos.x += TILESIZE;
        }
        if (transform.face_direction() & Transform::FrontFaceDirection::BACK) {
            new_pos.z -= TILESIZE;
        }
        if (transform.face_direction() & Transform::FrontFaceDirection::LEFT) {
            new_pos.x -= TILESIZE;
        }

        OptEntity furniture =
            EntityHelper::getEntityForID(can_hold_furniture.furniture_id());
        furniture->get<Transform>().update(new_pos);
    }
};

}  // namespace system_manager
