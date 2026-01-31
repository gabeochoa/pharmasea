#pragma once

#include "../../ah.h"
#include "../../components/can_hold_item.h"
#include "../../components/can_order_drink.h"
#include "../../components/can_pathfind.h"
#include "../../components/has_ai_drink_state.h"
#include "../../components/has_ai_queue_state.h"
#include "../../components/has_ai_target_entity.h"
#include "../../components/has_ai_target_location.h"
#include "../../components/has_patience.h"
#include "../../components/has_speech_bubble.h"
#include "../../components/is_ai_controlled.h"
#include "../../components/is_bank.h"
#include "../../components/is_drink.h"
#include "../../engine/statemanager.h"
#include "ai_entity_helpers.h"
#include "ai_shared_utilities.h"
#include "ai_system.h"
#include "ai_tags.h"
#include "ai_targeting.h"

namespace system_manager {

struct AIAtRegisterWaitForDrinkSystem
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
        if (ctrl.state != IsAIControlled::State::AtRegisterWaitForDrink) return;
        if (entity.is_missing<CanOrderDrink>()) return;
        if (!system_manager::ai::ai_tick_with_cooldown(entity, dt, 0.50f))
            return;

        HasAITargetEntity& tgt = entity.get<HasAITargetEntity>();
        OptEntity opt_reg = tgt.entity.resolve();
        if (!opt_reg) {
            tgt.entity.clear();
            request_next_state(entity, ctrl,
                               IsAIControlled::State::QueueForRegister);
            return;
        }
        Entity& reg = opt_reg.asE();

        CanOrderDrink& canOrderDrink = entity.get<CanOrderDrink>();
        VALIDATE(canOrderDrink.has_order(), "customer should have an order");

        CanHoldItem& regCHI = reg.get<CanHoldItem>();
        if (regCHI.empty()) return;

        OptEntity drink_opt = regCHI.item();
        if (!drink_opt) {
            regCHI.update(nullptr, entity_id::INVALID);
            return;
        }

        Item& drink = drink_opt.asE();
        if (!check_if_drink(drink)) return;

        std::string drink_name =
            drink.get<IsDrink>().underlying.has_value()
                ? std::string(magic_enum::enum_name(
                      drink.get<IsDrink>().underlying.value()))
                : "unknown";

        Drink orderedDrink = canOrderDrink.order();
        bool was_drink_correct = system_manager::ai::validate_drink_order(
            entity, orderedDrink, drink);
        if (!was_drink_correct) return;

        auto [price, tip] = get_price_for_order(
            {.order = canOrderDrink.get_order(),
             .max_pathfind_distance =
                 (int) entity.get<CanPathfind>().get_max_length(),
             .patience_pct = entity.get<HasPatience>().pct()});

        canOrderDrink.increment_tab(price);
        canOrderDrink.increment_tip(tip);
        canOrderDrink.apply_tip_multiplier(
            drink.get<IsDrink>().get_tip_multiplier());

        CanHoldItem& ourCHI = entity.get<CanHoldItem>();
        ourCHI.update(drink, entity.id);
        regCHI.update(nullptr, entity_id::INVALID);

        HasAIQueueState& qs = entity.get<HasAIQueueState>();
        system_manager::ai::line_leave(qs.line_wait, reg, entity);

        canOrderDrink.order_state = CanOrderDrink::OrderState::DrinkingNow;
        entity.get<HasSpeechBubble>().off();
        entity.get<HasPatience>().disable();
        entity.get<HasPatience>().reset();

        tgt.entity.clear();
        system_manager::ai::reset_component<HasAIQueueState>(entity);

        entity.removeComponentIfExists<HasAITargetLocation>();
        system_manager::ai::reset_component<HasAIDrinkState>(entity);

        request_next_state(entity, ctrl, IsAIControlled::State::Drinking);
    }
};

}  // namespace system_manager