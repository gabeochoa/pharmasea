#pragma once

#include "../../../ah.h"
#include "../../../components/has_day_night_timer.h"
#include "../../../components/is_ai_controlled.h"
#include "../../../engine/statemanager.h"
#include "../../../entity_helper.h"
#include "../ai_system.h"
#include "../ai_tags.h"

namespace system_manager {

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

}  // namespace system_manager