#pragma once

#include "../../ah.h"
#include "../../components/can_pathfind.h"
#include "../../components/has_ai_target_location.h"
#include "../../components/has_ai_wander_state.h"
#include "../../components/is_ai_controlled.h"
#include "../../components/is_round_settings_manager.h"
#include "../../engine/statemanager.h"
#include "../../entities/entity_helper.h"
#include "ai_entity_helpers.h"
#include "ai_shared_utilities.h"
#include "ai_tags.h"
#include "ai_targeting.h"

namespace system_manager {

struct AIWanderSystem
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
        if (ctrl.state != IsAIControlled::State::Wander) return;

        (void) system_manager::ai::ai_tick_with_cooldown(entity, dt, 0.25f);

        HasAITargetLocation& tgt = entity.get<HasAITargetLocation>();
        HasAIWanderState& ws = entity.get<HasAIWanderState>();

        if (!tgt.pos.has_value()) {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const IsRoundSettingsManager& irsm =
                sophie.get<IsRoundSettingsManager>();

            float max_dwell_time = irsm.get<float>(ConfigKey::MaxDwellTime);
            float dwell_time =
                RandomEngine::get().get_float(1.f, max_dwell_time);
            ws.timer.set_time(dwell_time);

            tgt.pos =
                system_manager::ai::pick_random_walkable_near(entity).value_or(
                    entity.get<Transform>().as2());
        }

        bool reached = pathfind.travel_toward(
            tgt.pos.value(),
            system_manager::ai::get_speed_for_entity(entity) * dt);
        if (!reached) return;

        if (!ws.timer.pass_time(dt)) return;

        tgt.pos.reset();
        request_next_state(entity, ctrl, ctrl.resume_state);
    }
};

}  // namespace system_manager