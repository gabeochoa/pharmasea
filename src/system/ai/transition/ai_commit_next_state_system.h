#pragma once

#include "../../../ah.h"
#include "../../../components/has_ai_bathroom_state.h"
#include "../../../components/is_ai_controlled.h"
#include "../../../engine/statemanager.h"
#include "../../../entities/entity_helper.h"
#include "../ai_entity_helpers.h"
#include "../ai_system.h"
#include "../ai_tags.h"

namespace system_manager {

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

}  // namespace system_manager