#pragma once

#include "../ah.h"
#include "../components/can_pathfind.h"
#include "../components/can_perform_job.h"
#include "../engine/statemanager.h"
#include "ai_system.h"

namespace system_manager {

// TODO should we have a separate system for each job type?
struct ProcessAiSystem : public afterhours::System<CanPathfind, CanPerformJob> {
    virtual bool should_run(const float) override {
        return GameState::get().is_game_like();
    }

    virtual void for_each_with(Entity& entity,
                               [[maybe_unused]] CanPathfind& canPathfind,
                               CanPerformJob& cpj, float dt) override {
        switch (cpj.current) {
            case Mopping:
                ai::process_ai_clean_vomit(entity, dt);
                break;
            case Drinking:
                ai::process_ai_drinking(entity, dt);
                break;
            case Bathroom:
                ai::process_ai_use_bathroom(entity, dt);
                break;
            case WaitInQueue:
                ai::process_ai_waitinqueue(entity, dt);
                break;
            case Leaving:
                ai::process_ai_leaving(entity, dt);
                break;
            case Paying:
                ai::process_ai_paying(entity, dt);
                break;
            case PlayJukebox:
                ai::process_jukebox_play(entity, dt);
                break;
            case Wandering:
                ai::process_wandering(entity, dt);
                break;
            case NoJob:
            case Wait:
            case EnterStore:
            case WaitInQueueForPickup:
            case MAX_JOB_TYPE:
                break;
        }
    }
};

}  // namespace system_manager
