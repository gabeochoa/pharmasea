#pragma once

#include "../../ah.h"
#include "../../components/can_order_drink.h"
#include "../../components/can_pathfind.h"
#include "../../components/has_ai_pay_state.h"
#include "../../components/has_ai_target_entity.h"
#include "../../components/is_ai_controlled.h"
#include "../../components/is_bank.h"
#include "../../components/is_round_settings_manager.h"
#include "../../components/transform.h"
#include "../../engine/statemanager.h"
#include "../../entities/entity_helper.h"
#include "ai_entity_helpers.h"
#include "ai_shared_utilities.h"
#include "ai_tags.h"
#include "ai_targeting.h"

namespace system_manager {

struct AIPaySystem
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
        if (ctrl.state != IsAIControlled::State::Pay) return;
        if (entity.is_missing<CanOrderDrink>()) return;
        if (!system_manager::ai::ai_tick_with_cooldown(entity, dt, 0.10f))
            return;

        HasAITargetEntity& tgt = entity.get<HasAITargetEntity>();
        HasAIPayState& ps = entity.get<HasAIPayState>();

        if (!system_manager::ai::entity_ref_valid(tgt.entity)) {
            OptEntity best =
                system_manager::ai::find_best_register_with_space(entity);
            if (!best) {
                wander_pause(entity, IsAIControlled::State::Pay);
                return;
            }
            tgt.entity.set(best.asE());
            system_manager::ai::line_add_to_queue(entity, ps.line_wait,
                                                  best.asE());
        }

        OptEntity opt_reg = tgt.entity.resolve();
        if (!opt_reg) {
            tgt.entity.clear();
            return;
        }
        Entity& reg = opt_reg.asE();
        entity.get<Transform>().turn_to_face_pos(reg.get<Transform>().as2());

        bool reached_front = system_manager::ai::line_try_to_move_closer(
            ps.line_wait, reg, entity,
            system_manager::ai::get_speed_for_entity(entity) * dt, [&]() {
                if (!ps.timer.initialized) {
                    Entity& sophie =
                        EntityHelper::getNamedEntity(NamedEntity::Sophie);
                    const IsRoundSettingsManager& irsm =
                        sophie.get<IsRoundSettingsManager>();
                    float pay_process_time =
                        irsm.get<float>(ConfigKey::PayProcessTime);
                    ps.timer.set_time(pay_process_time);
                }
            });
        if (!reached_front) return;

        if (!ps.timer.pass_time(dt)) return;

        Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
        CanOrderDrink& cod = entity.get<CanOrderDrink>();
        {
            IsBank& bank = sophie.get<IsBank>();
            bank.deposit_with_tip(cod.get_current_tab(), cod.get_current_tip());
            cod.clear_tab_and_tip();
        }

        system_manager::ai::line_leave(ps.line_wait, reg, entity);
        tgt.entity.clear();
        system_manager::ai::reset_component<HasAIPayState>(entity);

        request_next_state(entity, ctrl, IsAIControlled::State::Leave);
    }
};

}  // namespace system_manager