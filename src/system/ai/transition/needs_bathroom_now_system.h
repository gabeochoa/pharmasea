#pragma once

#include "../../../ah.h"
#include "../../../components/has_day_night_timer.h"
#include "../../../components/is_ai_controlled.h"
#include "../../../engine/statemanager.h"
#include "../../../entities/entity_helper.h"
#include "../ai_entity_helpers.h"
#include "../ai_system.h"
#include "../ai_tags.h"

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

}  // namespace system_manager