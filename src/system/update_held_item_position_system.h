#pragma once

#include "../ah.h"
#include "../components/can_hold_item.h"
#include "../components/custom_item_position.h"
#include "../components/transform.h"
#include "system_manager.h"

namespace system_manager {

// Forward declarations for functions defined in system_manager.cpp
vec3 get_new_held_position_custom(Entity& entity);
vec3 get_new_held_position_default(Entity& entity);

// Struct definition moved to afterhours_sixtyfps_systems.cpp

}  // namespace system_manager
