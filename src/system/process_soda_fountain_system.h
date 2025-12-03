#pragma once

#include "../ah.h"
#include "../components/can_hold_item.h"
#include "../components/is_drink.h"
#include "../entity_helper.h"
#include "system_manager.h"

namespace system_manager {

// Forward declaration for helper function in system_manager.cpp
void process_soda_fountain(Entity& entity, float dt);

// Struct definition moved to afterhours_sixtyfps_systems.cpp

}  // namespace system_manager
