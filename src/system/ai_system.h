
#pragma once

#include "../ah.h"
#include "../components/can_pathfind.h"
#include "../components/is_ai_controlled.h"
#include "../entity.h"
#include "../entity_query.h"

namespace system_manager::ai {

bool validate_drink_order(const Entity& customer, Drink orderedDrink,
                          Item& madeDrink);
float get_speed_for_entity(Entity& entity);

// Register AI-related afterhours systems.
void register_ai_systems(afterhours::SystemManager& systems);

}  // namespace system_manager::ai
