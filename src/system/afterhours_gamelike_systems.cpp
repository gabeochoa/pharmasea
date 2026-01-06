#include "../ah.h"
#include "../components/can_hold_item.h"
#include "../components/can_order_drink.h"
#include "../components/can_pathfind.h"
#include "../components/collects_customer_feedback.h"
#include "../components/has_day_night_timer.h"
#include "../components/has_ai_bathroom_state.h"
#include "../components/has_ai_drink_state.h"
#include "../components/has_ai_jukebox_state.h"
#include "../components/has_ai_pay_state.h"
#include "../components/has_ai_queue_state.h"
#include "../components/has_ai_target_entity.h"
#include "../components/has_ai_target_location.h"
#include "../components/has_ai_wander_state.h"
#include "../components/is_ai_controlled.h"
#include "../components/is_bank.h"
#include "../components/is_item_container.h"
#include "../components/is_pnumatic_pipe.h"
#include "../components/is_progression_manager.h"
#include "../components/is_round_settings_manager.h"
#include "../engine/runtime_globals.h"
#include "../engine/log.h"
#include "../engine/statemanager.h"
#include "../entity_helper.h"
#include "../entity_query.h"
#include "../entity_type.h"
#include "../globals.h"
#include "../vec_util.h"
#include "afterhours_systems.h"
#include "ai_entity_helpers.h"
#include "ai_system.h"
#include "ai_tags.h"
#include "ai_targeting.h"
#include "progression.h"
#include "sophie.h"
#include "system_manager.h"

namespace system_manager {

// System for processing containers that should backfill items during gamelike
// updates
struct ProcessIsContainerAndShouldBackfillItemSystem
    : public afterhours::System<IsItemContainer, CanHoldItem> {
    virtual ~ProcessIsContainerAndShouldBackfillItemSystem() = default;

    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& timer = sophie.get<HasDayNightTimer>();
            // Skip during transitions to avoid creating hundreds of items
            // before transition logic completes
            return !timer.needs_to_process_change;
        } catch (...) {
            return true;
        }
    }

    // TODO fold in function implementation
    virtual void for_each_with(Entity& entity, IsItemContainer& iic,
                               CanHoldItem& canHold, float dt) override {
        (void) iic;      // Unused parameter
        (void) canHold;  // Unused parameter
        process_is_container_and_should_backfill_item(entity, dt);
    }
};

struct RunTimerSystem : public afterhours::System<HasDayNightTimer> {
    virtual bool should_run(const float) override {
        return GameState::get().is_game_like();
    }

    virtual void for_each_with(Entity& entity, HasDayNightTimer& ht,
                               float dt) override {
        ht.pass_time(dt);
        if (!ht.is_round_over()) return;

        if (ht.is_bar_open()) {
            ht.close_bar();

            // TODO we assume that this entity is the same one which should be
            // the case if so then we should be using the System filtering to
            // just grab that for now lets just leave this and we can later
            // bring it in
            if (entity.is_missing<IsBank>())
                log_warn("system_manager::run_timer missing IsBank");

            if (ht.days_until() <= 0) {
                IsBank& isbank = entity.get<IsBank>();
                if (isbank.balance() < ht.rent_due()) {
                    log_error("you ran out of money, sorry");
                }
                isbank.withdraw(ht.rent_due());
                ht.reset_rent_days();
                // TODO update rent due amount
                ht.update_amount_due(ht.rent_due() + 25);

                // TODO add a way to pay ahead of time ?? design
            }

            return;
        }

        // TODO we assume that this entity is the same one which should be the
        // case if so then we should be using the System filtering to just grab
        // that for now lets just leave this and we can later bring it in
        if (entity.is_missing<IsProgressionManager>())
            log_warn("system_manager::run_timer missing IsProgressionManager");
        progression::collect_progression_options(entity, dt);

        // TODO theoretically we shouldnt start until after you choose upgrades
        // but we are gonna change how this works later anyway i think
        ht.open_bar();
    }
};

struct ProcessPnumaticPipePairingSystem
    : public afterhours::System<IsPnumaticPipe> {
    virtual bool should_run(const float) override {
        return GameState::get().is_game_like();
    }

    virtual void for_each_with(Entity& entity, IsPnumaticPipe& ipp,
                               float) override {
        if (ipp.has_pair()) return;

        OptEntity other_pipe = EntityQuery()  //
                                   .whereNotMarkedForCleanup()
                                   .whereNotID(entity.id)
                                   .whereHasComponent<IsPnumaticPipe>()
                                   .whereLambda([](const Entity& pipe) {
                                       const IsPnumaticPipe& otherpp =
                                           pipe.get<IsPnumaticPipe>();
                                       // Find only the ones that dont have a
                                       // pair
                                       return !otherpp.has_pair();
                                   })
                                   .gen_first();

        if (other_pipe.has_value()) {
            IsPnumaticPipe& otherpp = other_pipe->get<IsPnumaticPipe>();
            otherpp.paired.set_id(entity.id);
            ipp.paired.set_id(other_pipe->id);
        }

        // still dont have a pair, we probably just have an odd number
        if (!ipp.has_pair()) return;
    }
};

struct PassTimeForTransactionAnimationSystem
    : public afterhours::System<IsBank> {
    virtual bool should_run(const float) override {
        return GameState::get().is_game_like();
    }

    virtual void for_each_with(Entity&, IsBank& bank, float dt) override {
        std::vector<IsBank::Transaction>& transactions =
            bank.get_transactions();

        // Remove any old ones
        remove_all_matching<IsBank::Transaction>(
            transactions, [](const IsBank::Transaction& transaction) {
                return transaction.remainingTime <= 0.f ||
                       transaction.amount == 0;
            });

        if (transactions.empty()) return;

        IsBank::Transaction& transaction = bank.get_next_transaction();
        transaction.remainingTime -= dt;
    }
};

// Local run-gating helpers for AI systems.
namespace ai {
namespace {
[[nodiscard]] const HasDayNightTimer* day_night_timer() {
    // Centralized Sophie + HasDayNightTimer lookup (used by many systems).
    OptEntity sophie_opt =
        EntityHelper::getPossibleNamedEntity(NamedEntity::Sophie);
    if (!sophie_opt) return nullptr;
    const Entity& sophie = sophie_opt.asE();
    if (sophie.is_missing<HasDayNightTimer>()) return nullptr;
    return &sophie.get<HasDayNightTimer>();
}
}  // namespace

[[nodiscard]] bool normal_round() {
    if (!GameState::get().is_game_like()) return false;
    const HasDayNightTimer* timer = day_night_timer();
    if (!timer) return true;  // Conservative default.
    // Only run during the active round, and skip while transitions are in-flight.
    return timer->is_bar_open() && !timer->needs_to_process_change;
}

[[nodiscard]] bool close_bar_transition() {
    if (!GameState::get().is_game_like()) return false;
    const HasDayNightTimer* timer = day_night_timer();
    if (!timer) return false;
    return timer->needs_to_process_change && timer->is_bar_closed();
}

[[nodiscard]] bool any() { return normal_round() || close_bar_transition(); }
}  // namespace ai

// Override: request Bathroom regardless of pending transition.
struct NeedsBathroomNowSystem : public afterhours::System<IsAIControlled> {
    bool should_run(const float) override { return ai::normal_round(); }

    void for_each_with(Entity& entity, IsAIControlled& ai, float) override {
        // Don't interrupt these states (matches previous logic).
        if (ai.state == IsAIControlled::State::Bathroom ||
            ai.state == IsAIControlled::State::Drinking ||
            ai.state == IsAIControlled::State::Leave)
            return;

        if (!system_manager::ai::needs_bathroom_now(entity)) return;

        // Override: clear any pending request and force Bathroom.
        ai.clear_next_state();
        (void) ai.set_next_state(IsAIControlled::State::Bathroom);
        entity.enableTag(afterhours::tags::AITag::AITransitionPending);
    }
};

// Applies staged AI transitions (next_state -> state) and sets reset tag.
// Uses tag filtering to only run for entities with pending transitions.
struct AICommitNextStateSystem
    : public afterhours::System<IsAIControlled,
                                afterhours::tags::Any<
                                    afterhours::tags::AITag::AITransitionPending>> {
    bool should_run(const float) override { return ai::any(); }

    void for_each_with(Entity& entity, IsAIControlled& ai, float) override {
        const bool has_next = ai.has_next_state();
        if (!has_next) {
            // Keep tags consistent: if something set the pending tag but didn't
            // actually set next_state, clear it so other systems can run again.
            entity.disableTag(afterhours::tags::AITag::AITransitionPending);
            return;
        }

        const IsAIControlled::State old_state = ai.state;
        const IsAIControlled::State desired = ai.next_state.value();

        // Special handling: entering Bathroom must preserve the "return-to"
        // state slot. We encode it into the bathroom state component at commit
        // time so the reset system won't clobber it.
        if (desired == IsAIControlled::State::Bathroom &&
            old_state != IsAIControlled::State::Bathroom) {
            // Preserve any previously-staged return-to intent if present;
            // otherwise default to returning to the old state.
            IsAIControlled::State return_to = old_state;
            if (entity.has<HasAIBathroomState>()) {
                return_to = entity.get<HasAIBathroomState>().next_state;
            }
            ai::reset_component<HasAIBathroomState>(entity);
            entity.get<HasAIBathroomState>().next_state = return_to;
        }

        ai.set_state_immediately(desired);
        ai.clear_next_state();

        entity.disableTag(afterhours::tags::AITag::AITransitionPending);
        // Only tag resets for real transitions; this avoids unnecessary
        // "reset/no-op" frames on same-state requests.
        if (old_state != desired) {
            entity.enableTag(afterhours::tags::AITag::AINeedsResetting);
        }
    }
};

// Force-leave override (end-of-round / close-bar transition).
// This uses should_run() so it only runs while force-leave is active.
struct AIForceLeaveCommitSystem : public afterhours::System<IsAIControlled> {
    bool should_run(const float) override {
        // Mirror day/night transition systems: "leaving in-round" happens while
        // processing the close-bar transition.
        return ai::close_bar_transition();
    }

    void for_each_with(Entity& entity, IsAIControlled& ai, float) override {
        ai.set_state_immediately(IsAIControlled::State::Leave);
        ai.clear_next_state();
        entity.disableTag(afterhours::tags::AITag::AITransitionPending);
        entity.enableTag(afterhours::tags::AITag::AINeedsResetting);
    }
};

// Performs "on-enter" resets once a new state has been committed.
//
// Note: afterhours tag filtering currently only applies on Apple platforms
// (see vendor/afterhours/src/core/system.h). We still guard at runtime on other
// platforms to avoid running resets every frame.
struct AIOnEnterResetSystem
    : public afterhours::System<
          IsAIControlled,
          afterhours::tags::Any<afterhours::tags::AITag::AINeedsResetting>> {
    bool should_run(const float) override { return ai::any(); }

    void for_each_with(Entity& entity, IsAIControlled& ai, float) override {
        // Keep this cheap and conservative; do not erase state that must carry
        // information across transitions (e.g. Bathroom's return-to slot).
        switch (ai.state) {
            case IsAIControlled::State::Wander: {
                entity.removeComponentIfExists<HasAITargetEntity>();
                entity.removeComponentIfExists<HasAITargetLocation>();
                ai::reset_component<HasAIWanderState>(entity);
            } break;
            case IsAIControlled::State::QueueForRegister: {
                entity.removeComponentIfExists<HasAITargetEntity>();
                entity.removeComponentIfExists<HasAITargetLocation>();
                ai::reset_component<HasAIQueueState>(entity);
            } break;
            case IsAIControlled::State::AtRegisterWaitForDrink: {
                // Keep target register entity if present; no blanket resets.
            } break;
            case IsAIControlled::State::Drinking: {
                entity.removeComponentIfExists<HasAITargetEntity>();
                entity.removeComponentIfExists<HasAITargetLocation>();
                ai::reset_component<HasAIDrinkState>(entity);
            } break;
            case IsAIControlled::State::Bathroom: {
                entity.removeComponentIfExists<HasAITargetEntity>();
                entity.removeComponentIfExists<HasAITargetLocation>();
                // Do NOT reset HasAIBathroomState here; commit encodes next_state.
            } break;
            case IsAIControlled::State::Pay: {
                entity.removeComponentIfExists<HasAITargetEntity>();
                entity.removeComponentIfExists<HasAITargetLocation>();
                ai::reset_component<HasAIPayState>(entity);
            } break;
            case IsAIControlled::State::PlayJukebox: {
                entity.removeComponentIfExists<HasAITargetEntity>();
                entity.removeComponentIfExists<HasAITargetLocation>();
                ai::reset_component<HasAIJukeboxState>(entity);
            } break;
            case IsAIControlled::State::CleanVomit: {
                entity.removeComponentIfExists<HasAITargetEntity>();
                entity.removeComponentIfExists<HasAITargetLocation>();
            } break;
            case IsAIControlled::State::Leave: {
                entity.removeComponentIfExists<HasAITargetEntity>();
                entity.removeComponentIfExists<HasAITargetLocation>();
            } break;
        }

        entity.disableTag(afterhours::tags::AITag::AINeedsResetting);
    }
};

// Centralized "progress or wander" fallback (replaces per-state fallbacks).
struct AIFallbackToWanderSystem
    : public afterhours::System<
          IsAIControlled, CanPathfind,
          afterhours::tags::None<afterhours::tags::AITag::AITransitionPending,
                                 afterhours::tags::AITag::AINeedsResetting>> {
    bool should_run(const float) override { return ai::normal_round(); }

    void for_each_with(Entity& entity, IsAIControlled& ai,
                       [[maybe_unused]] CanPathfind&, float) override {
        switch (ai.state) {
            case IsAIControlled::State::QueueForRegister: {
                if (system_manager::ai::find_best_register_with_space(entity))
                    return;
                ai.set_resume_state(IsAIControlled::State::QueueForRegister);
                ai.clear_next_state();
                (void) ai.set_next_state(IsAIControlled::State::Wander);
                entity.enableTag(afterhours::tags::AITag::AITransitionPending);
            } break;
            case IsAIControlled::State::Pay: {
                if (system_manager::ai::find_best_register_with_space(entity))
                    return;
                ai.set_resume_state(IsAIControlled::State::Pay);
                ai.clear_next_state();
                (void) ai.set_next_state(IsAIControlled::State::Wander);
                entity.enableTag(afterhours::tags::AITag::AITransitionPending);
            } break;
            case IsAIControlled::State::Bathroom: {
                // Safety fallback: if the entity can't reason about drinks,
                // don't keep it stuck in Bathroom.
                if (!entity.is_missing<CanOrderDrink>()) return;
                ai.set_resume_state(IsAIControlled::State::Wander);
                ai.clear_next_state();
                (void) ai.set_next_state(IsAIControlled::State::Wander);
                entity.enableTag(afterhours::tags::AITag::AITransitionPending);
            } break;
            default:
                break;
        }
    }
};

// ---- Per-state AI systems (one state processed per entity per tick) ----
struct AIWanderSystem
    : public afterhours::System<
          IsAIControlled, CanPathfind,
          afterhours::tags::None<afterhours::tags::AITag::AITransitionPending,
                                 afterhours::tags::AITag::AINeedsResetting>> {
    bool should_run(const float) override { return ai::normal_round(); }
    void for_each_with(Entity& entity, IsAIControlled& ai,
                       [[maybe_unused]] CanPathfind&, float dt) override {
        if (ai.state != IsAIControlled::State::Wander) return;
        system_manager::ai::process_state_wander(entity, ai, dt);
    }
};

struct AIQueueForRegisterSystem
    : public afterhours::System<
          IsAIControlled, CanPathfind,
          afterhours::tags::None<afterhours::tags::AITag::AITransitionPending,
                                 afterhours::tags::AITag::AINeedsResetting>> {
    bool should_run(const float) override { return ai::normal_round(); }
    void for_each_with(Entity& entity, IsAIControlled& ai,
                       [[maybe_unused]] CanPathfind&, float dt) override {
        if (ai.state != IsAIControlled::State::QueueForRegister) return;
        system_manager::ai::process_state_queue_for_register(entity, dt);
    }
};

struct AIAtRegisterWaitForDrinkSystem
    : public afterhours::System<
          IsAIControlled, CanPathfind,
          afterhours::tags::None<afterhours::tags::AITag::AITransitionPending,
                                 afterhours::tags::AITag::AINeedsResetting>> {
    bool should_run(const float) override { return ai::normal_round(); }
    void for_each_with(Entity& entity, IsAIControlled& ai,
                       [[maybe_unused]] CanPathfind&, float dt) override {
        if (ai.state != IsAIControlled::State::AtRegisterWaitForDrink) return;
        system_manager::ai::process_state_at_register_wait_for_drink(entity, dt);
    }
};

struct AIDrinkingSystem
    : public afterhours::System<
          IsAIControlled, CanPathfind,
          afterhours::tags::None<afterhours::tags::AITag::AITransitionPending,
                                 afterhours::tags::AITag::AINeedsResetting>> {
    bool should_run(const float) override { return ai::normal_round(); }
    void for_each_with(Entity& entity, IsAIControlled& ai,
                       [[maybe_unused]] CanPathfind&, float dt) override {
        if (ai.state != IsAIControlled::State::Drinking) return;
        system_manager::ai::process_state_drinking(entity, dt);
    }
};

struct AIPaySystem
    : public afterhours::System<
          IsAIControlled, CanPathfind,
          afterhours::tags::None<afterhours::tags::AITag::AITransitionPending,
                                 afterhours::tags::AITag::AINeedsResetting>> {
    bool should_run(const float) override { return ai::normal_round(); }
    void for_each_with(Entity& entity, IsAIControlled& ai,
                       [[maybe_unused]] CanPathfind&, float dt) override {
        if (ai.state != IsAIControlled::State::Pay) return;
        system_manager::ai::process_state_pay(entity, dt);
    }
};

struct AIPlayJukeboxSystem
    : public afterhours::System<
          IsAIControlled, CanPathfind,
          afterhours::tags::None<afterhours::tags::AITag::AITransitionPending,
                                 afterhours::tags::AITag::AINeedsResetting>> {
    bool should_run(const float) override { return ai::normal_round(); }
    void for_each_with(Entity& entity, IsAIControlled& ai,
                       [[maybe_unused]] CanPathfind&, float dt) override {
        if (ai.state != IsAIControlled::State::PlayJukebox) return;
        system_manager::ai::process_state_play_jukebox(entity, dt);
    }
};

struct AIBathroomSystem
    : public afterhours::System<
          IsAIControlled, CanPathfind,
          afterhours::tags::None<afterhours::tags::AITag::AITransitionPending,
                                 afterhours::tags::AITag::AINeedsResetting>> {
    bool should_run(const float) override { return ai::normal_round(); }
    void for_each_with(Entity& entity, IsAIControlled& ai,
                       [[maybe_unused]] CanPathfind&, float dt) override {
        if (ai.state != IsAIControlled::State::Bathroom) return;
        system_manager::ai::process_state_bathroom(entity, dt);
    }
};

struct AICleanVomitSystem
    : public afterhours::System<
          IsAIControlled, CanPathfind,
          afterhours::tags::None<afterhours::tags::AITag::AITransitionPending,
                                 afterhours::tags::AITag::AINeedsResetting>> {
    bool should_run(const float) override { return ai::normal_round(); }
    void for_each_with(Entity& entity, IsAIControlled& ai,
                       [[maybe_unused]] CanPathfind&, float dt) override {
        if (ai.state != IsAIControlled::State::CleanVomit) return;
        system_manager::ai::process_state_clean_vomit(entity, dt);
    }
};

struct AILeaveSystem
    : public afterhours::System<
          IsAIControlled, CanPathfind,
          afterhours::tags::None<afterhours::tags::AITag::AITransitionPending,
                                 afterhours::tags::AITag::AINeedsResetting>> {
    bool should_run(const float) override { return ai::any(); }
    void for_each_with(Entity& entity, IsAIControlled& ai,
                       [[maybe_unused]] CanPathfind&, float dt) override {
        if (ai.state != IsAIControlled::State::Leave) return;
        system_manager::ai::process_state_leave(entity, dt);
    }
};

namespace sophie {
// Forward declarations for sophie namespace functions
void customers_in_store(Entity& entity);
void holding_stolen_item(Entity& entity);
void garbage_in_store(Entity& entity);
void bar_not_clean(Entity& entity);
void overlapping_furniture(Entity& entity);
void forgot_item_in_spawn_area(Entity& entity);
void deleting_item_needed_for_recipe(Entity& entity);
void lightweight_map_validation(Entity& entity);
}  // namespace sophie

struct EndOfRoundCompletionValidationSystem
    : public afterhours::System<HasDayNightTimer, CollectsCustomerFeedback,
                                afterhours::tags::All<EntityType::Sophie>> {
    virtual bool should_run(const float) override {
        return GameState::get().is_game_like();
    }

    virtual void for_each_with(Entity& entity, HasDayNightTimer& ht,
                               CollectsCustomerFeedback& feedback,
                               float dt) override {
        const auto debug_mode_on =
            globals::debug_ui_enabled();

        // TODO i dont like that this is copy paste from layers/round_end
        if (SystemManager::get().is_bar_closed() &&
            ht.get_current_length() > 0 && !debug_mode_on)
            return;

        if (!feedback.waiting_time_pass(dt)) {
            return;
        }

        // doing it this way so that if we wanna make them return bool itll be
        // easy
        typedef std::function<void(Entity&)> WaitingFn;

        std::vector<WaitingFn> fns{
            sophie::customers_in_store,   //
            sophie::holding_stolen_item,  //
            sophie::garbage_in_store,     //
            // sophie::player_holding_furniture,         //
            sophie::bar_not_clean,                    //
            sophie::overlapping_furniture,            //
            sophie::forgot_item_in_spawn_area,        //
            sophie::deleting_item_needed_for_recipe,  //
            sophie::lightweight_map_validation,
        };

        for (const auto& fn : fns) {
            fn(entity);
        }
    }
};

}  // namespace system_manager

void SystemManager::register_gamelike_systems() {
    systems.register_update_system(
        std::make_unique<system_manager::RunTimerSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::ProcessPnumaticPipePairingSystem>());
    systems.register_update_system(
        std::make_unique<
            system_manager::ProcessIsContainerAndShouldBackfillItemSystem>());
    systems.register_update_system(
        std::make_unique<
            system_manager::PassTimeForTransactionAnimationSystem>());
    // Apply resets from previously committed transitions before running AI.
    systems.register_update_system(
        std::make_unique<system_manager::AIOnEnterResetSystem>());
    // Bathroom override can preempt other transitions.
    systems.register_update_system(
        std::make_unique<system_manager::NeedsBathroomNowSystem>());
    // Centralized fallback can request a wander pause before state systems run.
    systems.register_update_system(
        std::make_unique<system_manager::AIFallbackToWanderSystem>());
    // State-specific AI systems (replaces monolithic dispatcher).
    systems.register_update_system(
        std::make_unique<system_manager::AIWanderSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::AIQueueForRegisterSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::AIAtRegisterWaitForDrinkSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::AIDrinkingSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::AIPaySystem>());
    systems.register_update_system(
        std::make_unique<system_manager::AIPlayJukeboxSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::AIBathroomSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::AICleanVomitSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::AILeaveSystem>());
    // Commit staged transitions after AI has had a chance to request them.
    systems.register_update_system(
        std::make_unique<system_manager::AICommitNextStateSystem>());
    // Force-leave override runs after normal commits.
    systems.register_update_system(
        std::make_unique<system_manager::AIForceLeaveCommitSystem>());
    systems.register_update_system(
        std::make_unique<
            system_manager::EndOfRoundCompletionValidationSystem>());

    register_day_night_transition_systems();
}