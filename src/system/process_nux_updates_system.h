#pragma once

#include "../ah.h"
#include "../components/is_nux_manager.h"
#include "../components/is_round_settings_manager.h"
#include "../engine/statemanager.h"
#include "../entity_helper.h"
#include "../entity_query.h"
#include "system_manager.h"

namespace system_manager {

// Forward declaration for helper function in system_manager.cpp
bool _create_nuxes(Entity& entity);
void process_nux_updates(Entity& entity, float dt);

// Struct definition moved to afterhours_sixtyfps_systems.cpp

}  // namespace system_manager
