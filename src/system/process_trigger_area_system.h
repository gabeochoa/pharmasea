#pragma once

#include "../ah.h"
#include "../components/is_trigger_area.h"
#include "../entity_helper.h"
#include "system_manager.h"

namespace system_manager {

// Forward declarations for helper functions in system_manager.cpp
void update_dynamic_trigger_area_settings(Entity& entity, float dt);
void count_all_possible_trigger_area_entrants(Entity& entity, float dt);
void count_in_building_trigger_area_entrants(Entity& entity, float dt);
void count_trigger_area_entrants(Entity& entity, float dt);
void update_trigger_area_percent(Entity& entity, float dt);
void trigger_cb_on_full_progress(Entity& entity, float dt);

// Struct definitions moved to afterhours_sixtyfps_systems.cpp

}  // namespace system_manager
