#pragma once

#include "../ah.h"

namespace system_manager {

// Registers all AI state processing systems.
// Each AI state (Wander, QueueForRegister, Drinking, etc.) is handled by its
// own system, allowing for better separation of concerns and tag-based
// filtering.
void register_ai_systems(afterhours::SystemManager& systems);

}  // namespace system_manager
