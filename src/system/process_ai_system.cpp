#include "process_ai_system.h"

#include <set>

#include "../components/can_hold_item.h"
#include "../components/can_order_drink.h"
#include "../components/has_ai_bathroom_state.h"
#include "../components/has_ai_drink_state.h"
#include "../components/has_ai_jukebox_state.h"
#include "../components/has_ai_pay_state.h"
#include "../components/has_ai_queue_state.h"
#include "../components/has_ai_target_entity.h"
#include "../components/has_ai_target_location.h"
#include "../components/has_ai_wander_state.h"
#include "../components/has_base_speed.h"
#include "../components/has_last_interacted_customer.h"
#include "../components/has_patience.h"
#include "../components/has_speech_bubble.h"
#include "../components/has_waiting_queue.h"
#include "../components/has_work.h"
#include "../components/is_bank.h"
#include "../components/is_progression_manager.h"
#include "../components/is_round_settings_manager.h"
#include "../components/is_toilet.h"
#include "../components/transform.h"
#include "../engine/statemanager.h"
#include "../entity_helper.h"
#include "../entity_makers.h"
#include "../entity_query.h"
#include "../globals.h"
#include "ai_entity_helpers.h"
#include "ai_helper.h"
#include "ai_queue_helpers.h"
#include "ai_system.h"
#include "ai_tags.h"
#include "ai_targeting.h"

namespace system_manager {

// =============================================================================
// Shared AI helper functions
// =============================================================================

namespace {

void request_next_state(Entity& entity, IsAIControlled& ctrl,
                        IsAIControlled::State s,
                        bool override_existing = false) {
    if (override_existing) {
        ctrl.clear_next_state();
    }
    const bool set = ctrl.set_next_state(s);
    if (set) {
        entity.enableTag(afterhours::tags::AITag::AITransitionPending);
    }
}

void wander_pause(Entity& e, IsAIControlled::State resume) {
    IsAIControlled& ctrl = e.get<IsAIControlled>();
    ctrl.set_resume_state(resume);
    request_next_state(e, ctrl, IsAIControlled::State::Wander);
}

void set_new_customer_order(Entity& entity) {
    Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
    const IsProgressionManager& progressionManager =
        sophie.get<IsProgressionManager>();

    CanOrderDrink& cod = entity.get<CanOrderDrink>();
    cod.set_order(progressionManager.get_random_unlocked_drink());

    // Clear any old drinking target/timer.
    entity.removeComponentIfExists<HasAITargetLocation>();
    ai::reset_component<HasAIDrinkState>(entity);
}

}  // namespace

// =============================================================================
// AILeaveSystem - Handles entities walking to the exit
// =============================================================================

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
        (void) pathfind.travel_toward(vec2{GATHER_SPOT, GATHER_SPOT},
                                      ai::get_speed_for_entity(entity) * dt);
    }
};

// =============================================================================
// AIWanderSystem - Handles wandering/pause behavior
// =============================================================================

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

        (void) ai::ai_tick_with_cooldown(entity, dt, 0.25f);

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

            tgt.pos = ai::pick_random_walkable_near(entity).value_or(
                entity.get<Transform>().as2());
        }

        bool reached = pathfind.travel_toward(
            tgt.pos.value(), ai::get_speed_for_entity(entity) * dt);
        if (!reached) return;

        if (!ws.timer.pass_time(dt)) return;

        tgt.pos.reset();
        request_next_state(entity, ctrl, ctrl.resume_state);
    }
};

// =============================================================================
// AIQueueForRegisterSystem - Handles customers queuing at registers
// =============================================================================

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

        (void) ai::ai_tick_with_cooldown(entity, dt, 0.10f);
        if (entity.is_missing<CanOrderDrink>()) return;

        HasAITargetEntity& tgt = entity.get<HasAITargetEntity>();
        HasAIQueueState& qs = entity.get<HasAIQueueState>();

        if (!ai::entity_ref_valid(tgt.entity)) {
            OptEntity best = ai::find_best_register_with_space(entity);
            if (!best) {
                wander_pause(entity, IsAIControlled::State::QueueForRegister);
                return;
            }
            Entity& best_reg = best.asE();
            tgt.entity.set(best_reg);
            ai::line_add_to_queue(entity, qs.line_wait, best_reg);
            (void) ai::line_position_in_line(qs.line_wait, best_reg, entity);
        }

        OptEntity opt_reg = tgt.entity.resolve();
        if (!opt_reg) {
            tgt.entity.clear();
            return;
        }
        Entity& reg = opt_reg.asE();

        entity.get<Transform>().turn_to_face_pos(reg.get<Transform>().as2());

        bool reached_front = ai::line_try_to_move_closer(
            qs.line_wait, reg, entity, ai::get_speed_for_entity(entity) * dt);
        qs.line_wait.queue_index = qs.line_wait.previous_line_index;
        if (!reached_front) return;

        entity.get<HasSpeechBubble>().on();
        entity.get<HasPatience>().enable();

        request_next_state(entity, ctrl,
                           IsAIControlled::State::AtRegisterWaitForDrink);
    }
};

// =============================================================================
// AIAtRegisterWaitForDrinkSystem - Handles customers waiting for their drink
// =============================================================================

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
        if (!ai::ai_tick_with_cooldown(entity, dt, 0.50f)) return;

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
        bool was_drink_correct =
            ai::validate_drink_order(entity, orderedDrink, drink);
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
        ai::line_leave(qs.line_wait, reg, entity);

        canOrderDrink.order_state = CanOrderDrink::OrderState::DrinkingNow;
        entity.get<HasSpeechBubble>().off();
        entity.get<HasPatience>().disable();
        entity.get<HasPatience>().reset();

        tgt.entity.clear();
        ai::reset_component<HasAIQueueState>(entity);

        entity.removeComponentIfExists<HasAITargetLocation>();
        ai::reset_component<HasAIDrinkState>(entity);

        request_next_state(entity, ctrl, IsAIControlled::State::Drinking);
    }
};

// =============================================================================
// AIDrinkingSystem - Handles customers drinking
// =============================================================================

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
        (void) ai::ai_tick_with_cooldown(entity, dt, 0.25f);

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
            tgt.pos = ai::pick_random_walkable_near(entity).value_or(
                entity.get<Transform>().as2());
        }

        bool reached = pathfind.travel_toward(
            tgt.pos.value(), ai::get_speed_for_entity(entity) * dt);
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

// =============================================================================
// AIPaySystem - Handles customers paying
// =============================================================================

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
        if (!ai::ai_tick_with_cooldown(entity, dt, 0.10f)) return;

        HasAITargetEntity& tgt = entity.get<HasAITargetEntity>();
        HasAIPayState& ps = entity.get<HasAIPayState>();

        if (!ai::entity_ref_valid(tgt.entity)) {
            OptEntity best = ai::find_best_register_with_space(entity);
            if (!best) {
                wander_pause(entity, IsAIControlled::State::Pay);
                return;
            }
            tgt.entity.set(best.asE());
            ai::line_add_to_queue(entity, ps.line_wait, best.asE());
        }

        OptEntity opt_reg = tgt.entity.resolve();
        if (!opt_reg) {
            tgt.entity.clear();
            return;
        }
        Entity& reg = opt_reg.asE();
        entity.get<Transform>().turn_to_face_pos(reg.get<Transform>().as2());

        bool reached_front = ai::line_try_to_move_closer(
            ps.line_wait, reg, entity, ai::get_speed_for_entity(entity) * dt,
            [&]() {
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

        ai::line_leave(ps.line_wait, reg, entity);
        tgt.entity.clear();
        ai::reset_component<HasAIPayState>(entity);

        request_next_state(entity, ctrl, IsAIControlled::State::Leave);
    }
};

// =============================================================================
// AIPlayJukeboxSystem - Handles customers playing the jukebox
// =============================================================================

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
        if (!ai::ai_tick_with_cooldown(entity, dt, 0.10f)) return;

        HasAITargetEntity& tgt = entity.get<HasAITargetEntity>();
        HasAIJukeboxState& js = entity.get<HasAIJukeboxState>();

        if (!ai::entity_ref_valid(tgt.entity)) {
            OptEntity best = find_best_jukebox(entity);
            if (!best) {
                set_new_customer_order(entity);
                ai::reset_component<HasAIJukeboxState>(entity);
                request_next_state(entity, ctrl,
                                   IsAIControlled::State::QueueForRegister);
                return;
            }

            if (best->has<HasLastInteractedCustomer>() &&
                best->get<HasLastInteractedCustomer>().customer.id ==
                    entity.id) {
                set_new_customer_order(entity);
                ai::reset_component<HasAIJukeboxState>(entity);
                request_next_state(entity, ctrl,
                                   IsAIControlled::State::QueueForRegister);
                return;
            }

            tgt.entity.set(best.asE());
            ai::line_add_to_queue(entity, js.line_wait, best.asE());
        }

        OptEntity opt_j = tgt.entity.resolve();
        if (!opt_j) {
            tgt.entity.clear();
            return;
        }
        Entity& jukebox = opt_j.asE();
        entity.get<Transform>().turn_to_face_pos(
            jukebox.get<Transform>().as2());

        bool reached_front = ai::line_try_to_move_closer(
            js.line_wait, jukebox, entity,
            ai::get_speed_for_entity(entity) * dt, [&]() {
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

        ai::line_leave(js.line_wait, jukebox, entity);
        tgt.entity.clear();
        ai::reset_component<HasAIJukeboxState>(entity);

        set_new_customer_order(entity);
        request_next_state(entity, ctrl,
                           IsAIControlled::State::QueueForRegister);
    }
};

// =============================================================================
// AIBathroomSystem - Handles customers using the bathroom
// =============================================================================

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

        if (!ai::needs_bathroom_now(entity)) {
            HasAIBathroomState& bs = entity.get<HasAIBathroomState>();
            request_next_state(entity, ctrl, bs.next_state);
            return;
        }

        if (!ai::ai_tick_with_cooldown(entity, dt, 0.10f)) return;

        HasAIBathroomState& bs = entity.get<HasAIBathroomState>();
        HasAITargetEntity& tgt = entity.get<HasAITargetEntity>();

        if (!ai::entity_ref_valid(tgt.entity)) {
            OptEntity best = find_best_toilet(entity);
            if (!best) return;
            tgt.entity.set(best.asE());
            ai::line_add_to_queue(entity, bs.line_wait, best.asE());
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
            ai::line_leave(bs.line_wait, toilet, entity);
            tgt.entity.clear();
            entity.get<CanOrderDrink>().empty_bladder();
            istoilet.end_use();
            (void) pathfind.travel_toward(
                vec2{0, 0}, ai::get_speed_for_entity(entity) * dt);
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
        bool reached_front =
            ai::line_try_to_move_closer(bs.line_wait, toilet, entity,
                                        ai::get_speed_for_entity(entity) * dt);
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

// =============================================================================
// AICleanVomitSystem - Handles mop boys cleaning vomit
// =============================================================================

struct AICleanVomitSystem
    : public afterhours::System<
          IsAIControlled, CanPathfind,
          afterhours::tags::None<afterhours::tags::AITag::AITransitionPending,
                                 afterhours::tags::AITag::AINeedsResetting>> {
    bool should_run(const float) override {
        return GameState::get().is_game_like();
    }

    void wander_in_bar(Entity& entity, CanPathfind& pathfind, float dt) {
        HasAITargetLocation& roam = entity.get<HasAITargetLocation>();
        if (!roam.pos.has_value()) {
            roam.pos =
                ai::pick_random_walkable_in_building(entity, BAR_BUILDING)
                    .value_or(entity.get<Transform>().as2());
        }
        bool reached = pathfind.travel_toward(
            roam.pos.value(), ai::get_speed_for_entity(entity) * dt);
        if (reached) roam.pos.reset();
    }

    void for_each_with(Entity& entity, IsAIControlled& ctrl,
                       CanPathfind& pathfind, float dt) override {
#if !__APPLE__
        if (entity.hasTag(afterhours::tags::AITag::AITransitionPending)) return;
        if (entity.hasTag(afterhours::tags::AITag::AINeedsResetting)) return;
#endif
        if (ctrl.state != IsAIControlled::State::CleanVomit) return;
        if (!ctrl.has_ability(IsAIControlled::AbilityCleanVomit)) return;

        HasAITargetEntity& tgt = entity.get<HasAITargetEntity>();

        if (!ai::entity_ref_valid(tgt.entity)) {
            auto other_ais = EntityQuery()
                                 .whereNotID(entity.id)
                                 .whereHasComponent<IsAIControlled>()
                                 .whereLambda([](const Entity& e) {
                                     if (e.is_missing<IsAIControlled>())
                                         return false;
                                     return e.get<IsAIControlled>().has_ability(
                                         IsAIControlled::AbilityCleanVomit);
                                 })
                                 .gen();
            std::set<int> existing_targets;
            for (const Entity& mop : other_ais) {
                if (mop.is_missing<HasAITargetEntity>()) continue;
                const EntityRef& r = mop.get<HasAITargetEntity>().entity;
                if (r.empty()) continue;
                existing_targets.insert(static_cast<int>(r.id));
            }

            bool more_boys_than_vomit =
                existing_targets.size() < other_ais.size();

            OptEntity vomit = EntityQuery()
                                  .whereType(EntityType::Vomit)
                                  .whereLambda([&](const Entity& v) {
                                      if (more_boys_than_vomit) return true;
                                      return !existing_targets.contains(v.id);
                                  })
                                  .orderByDist(entity.get<Transform>().as2())
                                  .gen_first();
            if (!vomit) {
                wander_in_bar(entity, pathfind, dt);
                return;
            }
            tgt.entity.set(vomit.asE());
            entity.removeComponentIfExists<HasAITargetLocation>();
        }

        OptEntity vomit = tgt.entity.resolve();
        if (!vomit) {
            tgt.entity.clear();
            wander_in_bar(entity, pathfind, dt);
            return;
        }

        bool reached =
            pathfind.travel_toward(vomit->get<Transform>().as2(),
                                   ai::get_speed_for_entity(entity) * dt);
        if (!reached) return;

        if (!vomit->has<HasWork>()) return;
        HasWork& vomWork = vomit->get<HasWork>();
        vomWork.call(vomit.asE(), entity, dt);
        if (vomit->cleanup) {
            tgt.entity.clear();
        }
    }
};

// =============================================================================
// Registration
// =============================================================================

void register_ai_systems(afterhours::SystemManager& systems) {
    systems.register_update_system(std::make_unique<AIWanderSystem>());
    systems.register_update_system(
        std::make_unique<AIQueueForRegisterSystem>());
    systems.register_update_system(
        std::make_unique<AIAtRegisterWaitForDrinkSystem>());
    systems.register_update_system(std::make_unique<AIDrinkingSystem>());
    systems.register_update_system(std::make_unique<AIPaySystem>());
    systems.register_update_system(std::make_unique<AIPlayJukeboxSystem>());
    systems.register_update_system(std::make_unique<AIBathroomSystem>());
    systems.register_update_system(std::make_unique<AICleanVomitSystem>());
    systems.register_update_system(std::make_unique<AILeaveSystem>());
}

}  // namespace system_manager
