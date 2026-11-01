#pragma once

#include "../../ah.h"
#include "../../components/can_pathfind.h"
#include "../../components/is_ai_controlled.h"
#include "../../engine/statemanager.h"
#include "ai_targeting.h"

namespace system_manager {

struct AILeaveSystem
    : public afterhours::System<
          IsAIControlled, CanPathfind,
          afterhours::tags::None<afterhours::tags::AITag::AITransitionPending,
                                 afterhours::tags::AITag::AINeedsResetting>> {
    bool should_run(const float) override {
        return GameState::get().is_game_like();
    }

    void for_each_with(Entity& entity, IsAIControlled& ctrl,
                       CanPathfind& pathfind, float dt) override {
#if !__APPLE__
        if (entity.hasTag(afterhours::tags::AITag::AITransitionPending)) return;
        if (entity.hasTag(afterhours::tags::AITag::AINeedsResetting)) return;
#endif
        if (ctrl.state != IsAIControlled::State::Leave) return;

        // TODO check the return value here and if true, stop running the
        // pathfinding
        // ^ does this mean just dynamically remove CanPathfind from the
        // customer entity?
        //
        // I noticed this during profiling :)
        //
        (void) pathfind.travel_toward(
            vec2{GATHER_SPOT, GATHER_SPOT},
            system_manager::ai::get_speed_for_entity(entity) * dt);
    }
};

}  // namespace system_manager