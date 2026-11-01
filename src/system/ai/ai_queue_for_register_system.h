#pragma once

#include "../../ah.h"
#include "../../components/can_order_drink.h"
#include "../../components/can_pathfind.h"
#include "../../components/has_ai_queue_state.h"
#include "../../components/has_ai_target_entity.h"
#include "../../components/is_ai_controlled.h"
#include "../../components/transform.h"
#include "../../engine/statemanager.h"
#include "ai_entity_helpers.h"
#include "ai_shared_utilities.h"
#include "ai_tags.h"
#include "ai_targeting.h"

namespace system_manager {

struct AIQueueForRegisterSystem
    : public afterhours::System<
          IsAIControlled, CanPathfind,
          afterhours::tags::None<afterhours::tags::AITag::AITransitionPending,
                                 afterhours::tags::AITag::AINeedsResetting>> {
    bool should_run(const float) override {
        return GameState::get().is_game_like();
    }

    void for_each_with(Entity& entity, IsAIControlled& ctrl,
                       [[maybe_unused]] CanPathfind&, float dt) override {
#if !__APPLE__
        if (entity.hasTag(afterhours::tags::AITag::AITransitionPending)) return;
        if (entity.hasTag(afterhours::tags::AITag::AINeedsResetting)) return;
#endif
        if (ctrl.state != IsAIControlled::State::QueueForRegister) return;

        (void) system_manager::ai::ai_tick_with_cooldown(entity, dt, 0.10f);
        if (entity.is_missing<CanOrderDrink>()) return;

        HasAITargetEntity& tgt = entity.get<HasAITargetEntity>();
        HasAIQueueState& qs = entity.get<HasAIQueueState>();

        if (!system_manager::ai::entity_ref_valid(tgt.entity)) {
            OptEntity best =
                system_manager::ai::find_best_register_with_space(entity);
            if (!best) {
                wander_pause(entity, IsAIControlled::State::QueueForRegister);
                return;
            }
            Entity& best_reg = best.asE();
            tgt.entity.set(best_reg);
            system_manager::ai::line_add_to_queue(entity, qs.line_wait,
                                                  best_reg);
            (void) system_manager::ai::line_position_in_line(qs.line_wait,
                                                             best_reg, entity);
        }

        OptEntity opt_reg = tgt.entity.resolve();
        if (!opt_reg) {
            tgt.entity.clear();
            return;
        }
        Entity& reg = opt_reg.asE();

        entity.get<Transform>().turn_to_face_pos(reg.get<Transform>().as2());

        bool reached_front = system_manager::ai::line_try_to_move_closer(
            qs.line_wait, reg, entity,
            system_manager::ai::get_speed_for_entity(entity) * dt);
        qs.line_wait.queue_index = qs.line_wait.previous_line_index;
        if (!reached_front) return;

        entity.get<HasSpeechBubble>().on();
        entity.get<HasPatience>().enable();

        request_next_state(entity, ctrl,
                           IsAIControlled::State::AtRegisterWaitForDrink);
    }
};

}  // namespace system_manager