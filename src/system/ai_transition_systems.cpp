#include "ai_transition_systems.h"

#include "../components/has_ai_bathroom_state.h"
#include "../components/has_ai_drink_state.h"
#include "../components/has_ai_jukebox_state.h"
#include "../components/has_ai_pay_state.h"
#include "../components/has_ai_queue_state.h"
#include "../components/has_ai_target_entity.h"
#include "../components/has_ai_target_location.h"
#include "../components/has_ai_wander_state.h"
#include "../components/has_day_night_timer.h"
#include "../components/is_ai_controlled.h"
#include "../engine/statemanager.h"
#include "../entity_helper.h"
#include "ai_entity_helpers.h"
#include "ai_system.h"
#include "ai_tags.h"

namespace system_manager {

// Override: request Bathroom regardless of pending transition.
struct NeedsBathroomNowSystem : public afterhours::System<IsAIControlled> {
    bool should_run(const float) override {
        return GameState::get().is_game_like();
    }

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
    : public afterhours::System<
          IsAIControlled,
          afterhours::tags::Any<afterhours::tags::AITag::AITransitionPending>> {
    bool should_run(const float) override {
        return GameState::get().is_game_like();
    }

    void for_each_with(Entity& entity, IsAIControlled& ai, float) override {
#if !__APPLE__
        // Tag filtering is Apple-only in afterhours today; guard manually on
        // other platforms to avoid iterating/processing every AI entity.
        if (!entity.hasTag(afterhours::tags::AITag::AITransitionPending))
            return;
#endif
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
        if (desired == IsAIControlled::State::Bathroom) {
            ai::reset_component<HasAIBathroomState>(entity);
            entity.get<HasAIBathroomState>().next_state = old_state;
        }

        ai.set_state_immediately(desired);
        ai.clear_next_state();

        entity.disableTag(afterhours::tags::AITag::AITransitionPending);
        entity.enableTag(afterhours::tags::AITag::AINeedsResetting);
    }
};

// Force-leave override (end-of-round / close-bar transition).
// This uses should_run() so it only runs while force-leave is active.
struct AIForceLeaveCommitSystem : public afterhours::System<IsAIControlled> {
    bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& timer = sophie.get<HasDayNightTimer>();
            // Mirror existing day/night transition systems: "leaving in-round"
            // happens while processing the close-bar transition.
            // TODO: Refactor this duplicated day/night transition check into a
            // shared helper (many systems repeat this Sophie + HasDayNightTimer
            // lookup and condition).
            return timer.needs_to_process_change && timer.is_bar_closed();
        } catch (...) {
            return false;
        }
    }

    void for_each_with(Entity& entity, IsAIControlled& ai, float) override {
        ai.set_state_immediately(IsAIControlled::State::Leave);
        ai.clear_next_state();
        entity.disableTag(afterhours::tags::AITag::AITransitionPending);
        entity.enableTag(afterhours::tags::AITag::AINeedsResetting);
    }
};

// Ensures AI entities have the required components for their current state.
// - For newly-spawned entities: adds missing components
// - For entities with AINeedsResetting tag: resets components to clear stale
// data
struct AISetupStateComponentsSystem
    : public afterhours::System<IsAIControlled> {
    bool should_run(const float) override {
        return GameState::get().is_game_like();
    }

    void for_each_with(Entity& entity, IsAIControlled& ai, float) override {
        bool reset = entity.hasTag(afterhours::tags::AITag::AINeedsResetting);

        auto add_or_reset = [&]<typename T>() {
            if (reset) {
                ai::reset_component<T>(entity);
            } else {
                entity.addComponentIfMissing<T>();
            }
        };

        auto remove = [&]<typename T>() {
            entity.removeComponentIfExists<T>();
        };

        switch (ai.state) {
            case IsAIControlled::State::Wander:
                remove.template operator()<HasAITargetEntity>();
                add_or_reset.template operator()<HasAITargetLocation>();
                add_or_reset.template operator()<HasAIWanderState>();
                break;

            case IsAIControlled::State::QueueForRegister:
                remove.template operator()<HasAITargetLocation>();
                add_or_reset.template operator()<HasAITargetEntity>();
                add_or_reset.template operator()<HasAIQueueState>();
                break;

            case IsAIControlled::State::AtRegisterWaitForDrink:
                // Keep existing components from QueueForRegister
                entity.addComponentIfMissing<HasAITargetEntity>();
                entity.addComponentIfMissing<HasAIQueueState>();
                break;

            case IsAIControlled::State::Drinking:
                remove.template operator()<HasAITargetEntity>();
                add_or_reset.template operator()<HasAITargetLocation>();
                add_or_reset.template operator()<HasAIDrinkState>();
                break;

            case IsAIControlled::State::Bathroom:
                remove.template operator()<HasAITargetLocation>();
                add_or_reset.template operator()<HasAITargetEntity>();
                // Don't reset HasAIBathroomState - it holds next_state from
                // commit
                entity.addComponentIfMissing<HasAIBathroomState>();
                break;

            case IsAIControlled::State::Pay:
                remove.template operator()<HasAITargetLocation>();
                add_or_reset.template operator()<HasAITargetEntity>();
                add_or_reset.template operator()<HasAIPayState>();
                break;

            case IsAIControlled::State::PlayJukebox:
                remove.template operator()<HasAITargetLocation>();
                add_or_reset.template operator()<HasAITargetEntity>();
                add_or_reset.template operator()<HasAIJukeboxState>();
                break;

            case IsAIControlled::State::CleanVomit:
                add_or_reset.template operator()<HasAITargetEntity>();
                add_or_reset.template operator()<HasAITargetLocation>();
                break;

            case IsAIControlled::State::Leave:
                remove.template operator()<HasAITargetEntity>();
                remove.template operator()<HasAITargetLocation>();
                break;
        }

        if (reset) {
            entity.disableTag(afterhours::tags::AITag::AINeedsResetting);
        }
    }
};

void register_ai_transition_systems(afterhours::SystemManager& systems) {
    // Set up state components for AI entities (adds missing, resets on
    // transition).
    systems.register_update_system(
        std::make_unique<AISetupStateComponentsSystem>());
    // Bathroom override can preempt other transitions.
    systems.register_update_system(std::make_unique<NeedsBathroomNowSystem>());
}

void register_ai_transition_commit_systems(afterhours::SystemManager& systems) {
    // Commit staged transitions after AI has had a chance to request them.
    systems.register_update_system(std::make_unique<AICommitNextStateSystem>());
    // Force-leave override runs after normal commits.
    systems.register_update_system(
        std::make_unique<AIForceLeaveCommitSystem>());
}

}  // namespace system_manager
