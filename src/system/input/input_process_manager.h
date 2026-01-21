#pragma once

#include "../../engine/keymap.h"
#include "../../entity.h"
#include "../../vendor_include.h"
#include "is_collidable.h"

namespace afterhours {
struct SystemManager;
}  // namespace afterhours

namespace system_manager {

namespace input_process_manager {

void collect_user_input(Entity& entity, float dt);
void register_input_systems(afterhours::SystemManager& systems);

namespace planning {
void register_input_systems(afterhours::SystemManager& systems);

}  // namespace planning

namespace inround {

void register_input_systems(afterhours::SystemManager& systems);

}  // namespace inround

}  // namespace input_process_manager
}  // namespace system_manager
