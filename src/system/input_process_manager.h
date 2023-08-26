#pragma once

#include "../components/transform.h"
#include "../engine/keymap.h"
#include "../entity.h"

namespace system_manager {

void person_update_given_new_pos(int id, Transform& transform,
                                 std::shared_ptr<Entity> person, float,
                                 vec3 new_pos_x, vec3 new_pos_z);
namespace input_process_manager {

[[nodiscard]] bool is_collidable(Entity& entity, OptEntity other = {});

void collect_user_input(std::shared_ptr<Entity> entity, float dt);
void process_player_movement_input(std::shared_ptr<Entity> entity, float dt,
                                   InputName input_name, float input_amount);

namespace planning {

void rotate_furniture(const std::shared_ptr<Entity> player);

void drop_held_furniture(Entity& player);

// TODO grabbing reach needs to be better, you should be able to grab in the 8
// spots around you and just prioritize the one you are facing
//

void handle_grab_or_drop(const std::shared_ptr<Entity>& player);

}  // namespace planning

namespace inround {

void work_furniture(const std::shared_ptr<Entity> player, float frame_dt);

void handle_drop(const std::shared_ptr<Entity>& player);

void handle_grab(const std::shared_ptr<Entity>& player);
void handle_grab_or_drop(const std::shared_ptr<Entity>& player);

}  // namespace inround

void process_input(const std::shared_ptr<Entity> entity,
                   const UserInput& input);

}  // namespace input_process_manager
}  // namespace system_manager
