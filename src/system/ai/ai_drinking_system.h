#pragma once

#include "../../ah.h"
#include "../../components/can_hold_item.h"
#include "../../components/can_order_drink.h"
#include "../../components/can_pathfind.h"
#include "../../components/has_ai_drink_state.h"
#include "../../components/has_ai_target_location.h"
#include "../../components/is_ai_controlled.h"
#include "../../components/is_round_settings_manager.h"
#include "../../engine/statemanager.h"
#include "../../entities/entity_helper.h"
#include "ai_entity_helpers.h"
#include "ai_shared_utilities.h"
#include "ai_system.h"
#include "ai_tags.h"
#include "ai_targeting.h"

namespace system_manager {

struct AIDrinkingSystem
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
        if (ctrl.state != IsAIControlled::State::Drinking) return;
        if (entity.is_missing<CanOrderDrink>()) return;
        (void) system_manager::ai::ai_tick_with_cooldown(entity, dt, 0.25f);

        Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
        const IsRoundSettingsManager& irsm =
            sophie.get<IsRoundSettingsManager>();

        CanOrderDrink& cod = entity.get<CanOrderDrink>();
        if (cod.order_state != CanOrderDrink::OrderState::DrinkingNow) return;

        HasAITargetLocation& tgt = entity.get<HasAITargetLocation>();
        HasAIDrinkState& ds = entity.get<HasAIDrinkState>();

        if (!tgt.pos.has_value()) {
            float drink_time = irsm.get<float>(ConfigKey::MaxDrinkTime);
            drink_time += RandomEngine::get().get_float(0.1f, 1.f);
            ds.timer.set_time(drink_time);
            tgt.pos =
                system_manager::ai::pick_random_walkable_near(entity).value_or(
                    entity.get<Transform>().as2());
        }

        bool reached = pathfind.travel_toward(
            tgt.pos.value(),
            system_manager::ai::get_speed_for_entity(entity) * dt);
        if (!reached) return;

        if (!ds.timer.pass_time(dt)) return;

        CanHoldItem& chi = entity.get<CanHoldItem>();
        OptEntity held_opt = chi.item();
        if (held_opt) held_opt.asE().cleanup = true;
        chi.update(nullptr, entity_id::INVALID);

        cod.on_order_finished();
        tgt.pos.reset();

        bool want_another = cod.wants_more_drinks();
        if (!want_another) {
            cod.order_state = CanOrderDrink::OrderState::DoneDrinking;
            request_next_state(entity, ctrl, IsAIControlled::State::Pay);
            return;
        }

        bool jukebox_allowed =
            ctrl.has_ability(IsAIControlled::AbilityPlayJukebox);
        if (jukebox_allowed && RandomEngine::get().get_bool()) {
            request_next_state(entity, ctrl,
                               IsAIControlled::State::PlayJukebox);
            return;
        }

        set_new_customer_order(entity);
        request_next_state(entity, ctrl,
                           IsAIControlled::State::QueueForRegister);
    }
};

}  // namespace system_manager