#include "ai_state_transition_systems.h"

#include "../ah.h"
#include "../components/can_pathfind.h"
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
#include "../dataclass/names.h"
#include "../entity_helper.h"
#include "../entity_type.h"

#include "ai_entity_helpers.h"
#include "ai_transition_tags.h"

namespace system_manager::ai {

namespace {

[[nodiscard]] bool force_leave_active() {
    // Option A: query Sophie + HasDayNightTimer.
    try {
        Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
        const HasDayNightTimer& timer = sophie.get<HasDayNightTimer>();
        // When we're transitioning into "bar closed" (day), customers must
        // leave regardless of staged transitions.
        return timer.needs_to_process_change && timer.is_bar_closed();
    } catch (...) {
        return false;
    }
}

inline void clear_tag(Entity& e, afterhours::TagId id) {
    e.tags.reset(static_cast<size_t>(id));
}

inline void set_tag(Entity& e, afterhours::TagId id) {
    e.tags.set(static_cast<size_t>(id));
}

}  // namespace

struct AICommitNextStateSystem : public afterhours::System<IsAIControlled> {
    bool should_run(const float) override { return GameState::get().is_game_like(); }

    void for_each_with(Entity& entity, IsAIControlled& ctrl, float) override {
        const bool pending =
            entity.hasTag(afterhours::tags::AITag::AITransitionPending);
        const bool force_leave = force_leave_active();

        if (!pending && !force_leave) return;

        const IsAIControlled::State prev_state = ctrl.state;

        if (force_leave) {
            ctrl.state = IsAIControlled::State::Leave;
        } else if (ctrl.next_state.has_value()) {
            ctrl.state = ctrl.next_state.value();
        }

        // Clear staged state (even if force-leave overrides it).
        ctrl.next_state.reset();

        // Transition bookkeeping tags.
        clear_tag(entity, afterhours::tags::AITag::AITransitionPending);
        set_tag(entity, afterhours::tags::AITag::AINeedsResetting);

        // Preserve "return-to" for Bathroom enters (matches old behavior where
        // entering bathroom remembered the prior state).
        if (ctrl.state == IsAIControlled::State::Bathroom) {
            HasAIBathroomState& bs = ensure_component<HasAIBathroomState>(entity);
            bs.next_state = prev_state;
        }
    }
};

struct AIOnEnterResetSystem : public afterhours::System<IsAIControlled> {
    bool should_run(const float) override { return GameState::get().is_game_like(); }

    void for_each_with(Entity& entity, IsAIControlled& ctrl, float) override {
        if (!entity.hasTag(afterhours::tags::AITag::AINeedsResetting)) return;

        // Clear any stale targeting by default; state logic can recreate it.
        entity.removeComponentIfExists<HasAITargetEntity>();
        entity.removeComponentIfExists<HasAITargetLocation>();

        switch (ctrl.state) {
            case IsAIControlled::State::Wander:
                reset_component<HasAIWanderState>(entity);
                break;
            case IsAIControlled::State::QueueForRegister:
            case IsAIControlled::State::AtRegisterWaitForDrink:
                reset_component<HasAIQueueState>(entity);
                break;
            case IsAIControlled::State::Drinking:
                reset_component<HasAIDrinkState>(entity);
                break;
            case IsAIControlled::State::Bathroom: {
                // Preserve the return state if it was set during commit.
                HasAIBathroomState& bs = ensure_component<HasAIBathroomState>(entity);
                const IsAIControlled::State return_to = bs.next_state;
                reset_component<HasAIBathroomState>(entity);
                entity.get<HasAIBathroomState>().next_state = return_to;
            } break;
            case IsAIControlled::State::Pay:
                reset_component<HasAIPayState>(entity);
                break;
            case IsAIControlled::State::PlayJukebox:
                reset_component<HasAIJukeboxState>(entity);
                break;
            case IsAIControlled::State::CleanVomit:
                // Target is cleared above; no additional scratch needed.
                break;
            case IsAIControlled::State::Leave:
                // Ensure pathfinding exists for the leave behavior.
                if (entity.is_missing<CanPathfind>()) {
                    entity.addComponent<CanPathfind>().set_parent(entity.id);
                } else {
                    entity.get<CanPathfind>().set_parent(entity.id);
                }
                break;
        }

        // Done resetting.
        clear_tag(entity, afterhours::tags::AITag::AINeedsResetting);
    }
};

void register_ai_reset_system(afterhours::SystemManager& systems) {
    systems.register_update_system(std::make_unique<AIOnEnterResetSystem>());
}

void register_ai_commit_system(afterhours::SystemManager& systems) {
    systems.register_update_system(std::make_unique<AICommitNextStateSystem>());
}

}  // namespace system_manager::ai

