#pragma once

#include "../../ah.h"
#include "../../components/can_order_drink.h"
#include "../../components/can_pathfind.h"
#include "../../components/has_ai_jukebox_state.h"
#include "../../components/has_ai_target_entity.h"
#include "../../components/has_last_interacted_customer.h"
#include "../../components/has_waiting_queue.h"
#include "../../components/is_ai_controlled.h"
#include "../../components/is_bank.h"
#include "../../components/transform.h"
#include "../../engine/statemanager.h"
#include "../../entity_helper.h"
#include "../../entity_query.h"
#include "../../entity_type.h"
#include "ai_entity_helpers.h"
#include "ai_shared_utilities.h"
#include "ai_system.h"
#include "ai_tags.h"
#include "ai_targeting.h"

namespace system_manager {

struct AIPlayJukeboxSystem
    : public afterhours::System<
          IsAIControlled, CanPathfind,
          afterhours::tags::None<afterhours::tags::AITag::AITransitionPending,
                                 afterhours::tags::AITag::AINeedsResetting>> {
    bool should_run(const float) override {
        return GameState::get().is_game_like();
    }

    OptEntity find_best_jukebox(Entity& entity) {
        return EntityQuery()
            .whereType(EntityType::Jukebox)
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
                       [[maybe_unused]] CanPathfind&, float dt) override {
#if !__APPLE__
        if (entity.hasTag(afterhours::tags::AITag::AITransitionPending)) return;
        if (entity.hasTag(afterhours::tags::AITag::AINeedsResetting)) return;
#endif
        if (ctrl.state != IsAIControlled::State::PlayJukebox) return;
        if (entity.is_missing<CanOrderDrink>()) return;
        if (!system_manager::ai::ai_tick_with_cooldown(entity, dt, 0.10f))
            return;

        HasAITargetEntity& tgt = entity.get<HasAITargetEntity>();
        HasAIJukeboxState& js = entity.get<HasAIJukeboxState>();

        if (!system_manager::ai::entity_ref_valid(tgt.entity)) {
            OptEntity best = find_best_jukebox(entity);
            if (!best) {
                set_new_customer_order(entity);
                system_manager::ai::reset_component<HasAIJukeboxState>(entity);
                request_next_state(entity, ctrl,
                                   IsAIControlled::State::QueueForRegister);
                return;
            }

            if (best->has<HasLastInteractedCustomer>() &&
                best->get<HasLastInteractedCustomer>().customer.id ==
                    entity.id) {
                set_new_customer_order(entity);
                system_manager::ai::reset_component<HasAIJukeboxState>(entity);
                request_next_state(entity, ctrl,
                                   IsAIControlled::State::QueueForRegister);
                return;
            }

            tgt.entity.set(best.asE());
            system_manager::ai::line_add_to_queue(entity, js.line_wait,
                                                  best.asE());
        }

        OptEntity opt_j = tgt.entity.resolve();
        if (!opt_j) {
            tgt.entity.clear();
            return;
        }
        Entity& jukebox = opt_j.asE();
        entity.get<Transform>().turn_to_face_pos(
            jukebox.get<Transform>().as2());

        bool reached_front = system_manager::ai::line_try_to_move_closer(
            js.line_wait, jukebox, entity,
            system_manager::ai::get_speed_for_entity(entity) * dt, [&]() {
                if (!js.timer.initialized) {
                    js.timer.set_time(5.f);
                }
            });
        if (!reached_front) return;

        if (!js.timer.pass_time(dt)) return;

        {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            IsBank& bank = sophie.get<IsBank>();
            bank.deposit(10);
        }
        if (jukebox.has<HasLastInteractedCustomer>()) {
            jukebox.get<HasLastInteractedCustomer>().customer.set_id(entity.id);
        }

        system_manager::ai::line_leave(js.line_wait, jukebox, entity);
        tgt.entity.clear();
        system_manager::ai::reset_component<HasAIJukeboxState>(entity);

        set_new_customer_order(entity);
        request_next_state(entity, ctrl,
                           IsAIControlled::State::QueueForRegister);
    }
};

}  // namespace system_manager