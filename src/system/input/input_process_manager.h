#pragma once

#include "../../engine/keymap.h"
#include "../../entities/entity.h"
#include "../../vendor_include.h"
#include "is_collidable.h"

namespace afterhours {
struct SystemManager;
}  // namespace afterhours

namespace system_manager {

namespace input_process_manager {

void collect_user_input(Entity& entity, float dt);

namespace planning {
void register_input_systems(afterhours::SystemManager& systems);

}  // namespace planning

namespace inround {

void register_input_systems(afterhours::SystemManager& systems);

}  // namespace inround

void process_input(Entity& entity, const UserInputSnapshot& input);

}  // namespace input_process_manager
}  // namespace system_manager
