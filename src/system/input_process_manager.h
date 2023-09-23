#pragma once

#include "../components/transform.h"
#include "../engine/keymap.h"
#include "../entity.h"

namespace system_manager {

void person_update_given_new_pos(int id, Transform& transform, Entity& person,
                                 float, vec3 new_pos_x, vec3 new_pos_z);
namespace input_process_manager {

[[nodiscard]] bool is_collidable(const Entity& entity, OptEntity other = {});

void collect_user_input(std::shared_ptr<Entity> entity, float dt);
void process_player_movement_input(Entity& entity, float dt,
                                   InputName input_name, float input_amount);

namespace planning {

void rotate_furniture(const Entity& player);

void drop_held_furniture(Entity& player);

// TODO grabbing reach needs to be better, you should be able to grab in the 8
// spots around you and just prioritize the one you are facing
//

void handle_grab_or_drop(Entity& player);

}  // namespace planning

namespace inround {

void work_furniture(Entity& player, float frame_dt);
void handle_drop(Entity& player);
void handle_grab(Entity& player);
void handle_grab_or_drop(Entity& player);

}  // namespace inround

void process_input(Entity& entity, const UserInput& input);

}  // namespace input_process_manager
}  // namespace system_manager
