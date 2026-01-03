
#pragma once

#include "../components/can_pathfind.h"
#include "../components/is_ai_controlled.h"
#include "../entity.h"
#include "../entity_query.h"

namespace system_manager::ai {

bool validate_drink_order(const Entity& customer, Drink orderedDrink,
                          Item& madeDrink);
float get_speed_for_entity(Entity& entity);

void process_ai_entity(Entity& entity, float dt);

}  // namespace system_manager::ai
