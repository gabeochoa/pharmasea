#pragma once

#include "../../ah.h"
#include "../../components/can_order_drink.h"
#include "../../components/can_pathfind.h"
#include "../../components/has_ai_bathroom_state.h"
#include "../../components/has_ai_target_entity.h"
#include "../../components/has_waiting_queue.h"
#include "../../components/is_ai_controlled.h"
#include "../../components/is_round_settings_manager.h"
#include "../../components/is_toilet.h"
#include "../../engine/statemanager.h"
#include "../../entities/entity_helper.h"
#include "../../entities/entity_makers.h"
#include "../../entities/entity_query.h"
#include "../../entities/entity_type.h"
#include "ai_entity_helpers.h"
#include "ai_shared_utilities.h"
#include "ai_tags.h"
#include "ai_targeting.h"

namespace system_manager {

struct AIBathroomSystem
    : public afterhours::System<
          IsAIControlled, CanPathfind,
          afterhours::tags::None<afterhours::tags::AITag::AITransitionPending,
                                 afterhours::tags::AITag::AINeedsResetting>> {
    bool should_run(const float) override {
        return GameState::get().is_game_like();
    }

    OptEntity find_best_toilet(Entity& entity) {
        return EntityQuery()
            .whereHasComponent<IsToilet>()
            .whereHasComponent<HasWaitingQueue>()
            .whereLambda([](const Entity& e) {
                return !e.get<HasWaitingQueue>().is_full();
            })
            .whereCanPathfindTo(entity.get<Transform>().as2())
            .orderByLambda([](const Entity& r1, const Entity& r2) {
                return r1.get<HasWaitingQueue>().get_next_pos() <
                       r2.get<HasWaitingQueue>().get_next_pos();
            })
            .gen_first();
    }

    void for_each_with(Entity& entity, IsAIControlled& ctrl,
                       CanPathfind& pathfind, float dt) override {
#if !__APPLE__
        if (entity.hasTag(afterhours::tags::AITag::AITransitionPending)) return;
        if (entity.hasTag(afterhours::tags::AITag::AINeedsResetting)) return;
#endif
        if (ctrl.state != IsAIControlled::State::Bathroom) return;

        if (entity.is_missing<CanOrderDrink>()) {
            request_next_state(entity, ctrl, IsAIControlled::State::Wander);
            return;
        }

        if (!system_manager::ai::needs_bathroom_now(entity)) {
            HasAIBathroomState& bs = entity.get<HasAIBathroomState>();
            request_next_state(entity, ctrl, bs.next_state);
            return;
        }

        if (!system_manager::ai::ai_tick_with_cooldown(entity, dt, 0.10f))
            return;

        HasAIBathroomState& bs = entity.get<HasAIBathroomState>();
        HasAITargetEntity& tgt = entity.get<HasAITargetEntity>();

        if (!system_manager::ai::entity_ref_valid(tgt.entity)) {
            OptEntity best = find_best_toilet(entity);
            if (!best) return;
            tgt.entity.set(best.asE());
            system_manager::ai::line_add_to_queue(entity, bs.line_wait,
                                                  best.asE());
            bs.floor_timer.set_time(5.f);
        }

        OptEntity opt_toilet = tgt.entity.resolve();
        if (!opt_toilet) {
            tgt.entity.clear();
            return;
        }
        Entity& toilet = opt_toilet.asE();
        entity.get<Transform>().turn_to_face_pos(toilet.get<Transform>().as2());
        IsToilet& istoilet = toilet.get<IsToilet>();

        const auto on_finished = [&]() {
            system_manager::ai::line_leave(bs.line_wait, toilet, entity);
            tgt.entity.clear();
            entity.get<CanOrderDrink>().empty_bladder();
            istoilet.end_use();
            (void) pathfind.travel_toward(
                vec2{0, 0},
                system_manager::ai::get_speed_for_entity(entity) * dt);
            request_next_state(entity, ctrl, bs.next_state);
        };

        if (bs.floor_timer.pass_time(dt)) {
            auto& vom = EntityHelper::createEntity();
            furniture::make_vomit(
                vom, SpawnInfo{.location = entity.get<Transform>().as2(),
                               .is_first_this_round = false});
            on_finished();
            return;
        }

        int previous_position = bs.line_wait.previous_line_index;
        bool reached_front = system_manager::ai::line_try_to_move_closer(
            bs.line_wait, toilet, entity,
            system_manager::ai::get_speed_for_entity(entity) * dt);
        int new_position = bs.line_wait.previous_line_index;

        if (previous_position != new_position && bs.floor_timer.initialized) {
            float totalTime = bs.floor_timer.reset_to;
            (void) bs.floor_timer.pass_time(-1.f * totalTime * 0.1f);
        }

        if (!reached_front) return;

        bool not_me = !istoilet.available() && !istoilet.is_user(entity.id);
        if (not_me) return;

        bool we_are_using_it = istoilet.is_user(entity.id);
        if (!we_are_using_it) {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const IsRoundSettingsManager& irsm =
                sophie.get<IsRoundSettingsManager>();
            float piss_timer = irsm.get<float>(ConfigKey::PissTimer);
            bs.use_toilet_timer.set_time(piss_timer);
            istoilet.start_use(entity.id);
        }

        (void) bs.floor_timer.pass_time(-1.f * dt);

        if (bs.use_toilet_timer.pass_time(dt)) {
            on_finished();
        }
    }
};

}  // namespace system_manager