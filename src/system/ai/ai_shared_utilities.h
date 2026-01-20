#pragma once

#include "../../ah.h"
#include "../../components/can_order_drink.h"
#include "../../components/is_ai_controlled.h"
#include "../../components/is_progression_manager.h"
#include "../../entity_helper.h"
#include "ai_entity_helpers.h"
#include "ai_system.h"
#include "ai_tags.h"

namespace system_manager {

// =============================================================================
// Shared AI helper functions
// =============================================================================

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
    system_manager::ai::reset_component<HasAIDrinkState>(entity);
}

}  // namespace system_manager