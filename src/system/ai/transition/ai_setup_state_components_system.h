#pragma once

#include "../../../ah.h"
#include "../../../components/has_ai_bathroom_state.h"
#include "../../../components/has_ai_drink_state.h"
#include "../../../components/has_ai_jukebox_state.h"
#include "../../../components/has_ai_pay_state.h"
#include "../../../components/has_ai_queue_state.h"
#include "../../../components/has_ai_target_entity.h"
#include "../../../components/has_ai_target_location.h"
#include "../../../components/has_ai_wander_state.h"
#include "../../../components/is_ai_controlled.h"
#include "../../../engine/statemanager.h"
#include "../../../entities/entity_helper.h"
#include "../ai_entity_helpers.h"
#include "../ai_system.h"
#include "../ai_tags.h"

namespace system_manager {

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

}  // namespace system_manager