#include "process_ai_system.h"

#include "../engine/statemanager.h"
#include "ai_system.h"
#include "ai_tags.h"

namespace system_manager {

bool ProcessAiSystem::should_run(const float) {
    return GameState::get().is_game_like();
}

void ProcessAiSystem::for_each_with(Entity& entity, IsAIControlled& ctrl,
                                    [[maybe_unused]] CanPathfind&, float dt) {
#if !__APPLE__
    // If tag filtering is not active, guard manually.
    if (entity.hasTag(afterhours::tags::AITag::AITransitionPending)) return;
    if (entity.hasTag(afterhours::tags::AITag::AINeedsResetting)) return;
#endif
    switch (ctrl.state) {
        case IsAIControlled::State::Wander:
            ai::process_state_wander(entity, ctrl, dt);
            break;
        case IsAIControlled::State::QueueForRegister:
            ai::process_state_queue_for_register(entity, dt);
            break;
        case IsAIControlled::State::AtRegisterWaitForDrink:
            ai::process_state_at_register_wait_for_drink(entity, dt);
            break;
        case IsAIControlled::State::Drinking:
            ai::process_state_drinking(entity, dt);
            break;
        case IsAIControlled::State::Pay:
            ai::process_state_pay(entity, dt);
            break;
        case IsAIControlled::State::PlayJukebox:
            ai::process_state_play_jukebox(entity, dt);
            break;
        case IsAIControlled::State::Bathroom:
            ai::process_state_bathroom(entity, dt);
            break;
        case IsAIControlled::State::CleanVomit:
            ai::process_state_clean_vomit(entity, dt);
            break;
        case IsAIControlled::State::Leave:
            ai::process_state_leave(entity, dt);
            break;
    }
}

}  // namespace system_manager

