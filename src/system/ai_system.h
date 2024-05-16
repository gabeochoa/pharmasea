
#pragma once

#include "../components/can_pathfind.h"
#include "../components/can_perform_job.h"
#include "../entity.h"
#include "../entity_query.h"
#include "../job.h"

namespace system_manager {
namespace ai {

bool validate_drink_order(const Entity& customer, Drink orderedDrink,
                          Item& madeDrink);
float get_speed_for_entity(Entity& entity);

template<typename T>
inline void reset_job_component(Entity& entity) {
    entity.removeComponent<T>();
    entity.addComponent<T>();
}
void next_job(Entity& entity, JobType suggestion);
void process_ai_waitinqueue(Entity& entity, float dt);
void _set_customer_next_order(Entity& entity);
void process_wandering(Entity& entity, float dt);
void process_ai_drinking(Entity& entity, float dt);
void process_ai_clean_vomit(Entity& entity, float dt);
void process_ai_use_bathroom(Entity& entity, float dt);
void process_ai_leaving(Entity& entity, float dt);
void process_ai_paying(Entity& entity, float dt);
void process_jukebox_play(Entity& entity, float dt);

inline void process_(Entity& entity, float dt) {
    if (entity.is_missing<CanPathfind>()) return;

    switch (entity.get<CanPerformJob>().current) {
        case Mopping:
            process_ai_clean_vomit(entity, dt);
            break;
        case Drinking:
            process_ai_drinking(entity, dt);
            break;
        case Bathroom:
            process_ai_use_bathroom(entity, dt);
            break;
        case WaitInQueue:
            process_ai_waitinqueue(entity, dt);
            break;
        case Leaving:
            process_ai_leaving(entity, dt);
            break;
        case Paying:
            process_ai_paying(entity, dt);
            break;
        case PlayJukebox:
            process_jukebox_play(entity, dt);
            break;
        case Wandering:
            process_wandering(entity, dt);
            break;
        case NoJob:
        case Wait:
        case EnterStore:
        case WaitInQueueForPickup:
        case MAX_JOB_TYPE:
            break;
    }
}

}  // namespace ai
}  // namespace system_manager
