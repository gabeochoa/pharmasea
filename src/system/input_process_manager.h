#pragma once

#include "../engine/keymap.h"
#include "../entity.h"
#include "../vendor_include.h"

namespace afterhours {
struct SystemManager;
}  // namespace afterhours

namespace system_manager {

namespace input_process_manager {

[[nodiscard]] bool is_collidable(const Entity& entity, OptEntity other = {});
[[nodiscard]] bool is_collidable(const Entity& entity, const Entity& other);

void collect_user_input(Entity& entity, float dt);

namespace planning {
void register_input_systems(afterhours::SystemManager& systems);

}  // namespace planning

namespace inround {

void register_input_systems(afterhours::SystemManager& systems);

}  // namespace inround

void process_input(Entity& entity, const UserInput& input);

}  // namespace input_process_manager
}  // namespace system_manager
