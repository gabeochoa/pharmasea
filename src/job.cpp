
#include "job.h"

#include "components/can_order_drink.h"
#include "components/can_pathfind.h"
#include "components/can_perform_job.h"
#include "components/has_base_speed.h"
#include "components/has_patience.h"
#include "components/has_speech_bubble.h"
#include "components/has_timer.h"
#include "components/has_waiting_queue.h"
#include "components/is_bank.h"
#include "components/is_drink.h"
#include "components/is_progression_manager.h"
#include "components/is_round_settings_manager.h"
#include "components/is_toilet.h"
#include "engine/assert.h"
#include "engine/bfs.h"
#include "engine/pathfinder.h"
#include "entity.h"
#include "entity_helper.h"
#include "entity_query.h"
#include "globals.h"
#include "system/logging_system.h"

// inline Job* create_wandering_job(vec2 _start) {
// // TODO add cooldown so that not all time is spent here
// int max_tries = 10;
// int range = 20;
// bool walkable = false;
// int i = 0;
// vec2 target;
// while (!walkable) {
// target =
// (vec2){1.f * randIn(-range, range), 1.f * randIn(-range, range)};
// walkable = EntityHelper::isWalkable(target);
// i++;
// if (i > max_tries) {
// return nullptr;
// }
// }
// return new WanderingJob(_start, target);
// }
//

bool Job::is_at_position(const Entity& entity, vec2 position) {
    return vec::distance(entity.get<Transform>().as2(), position) <
           (TILESIZE / 2.f);
}
