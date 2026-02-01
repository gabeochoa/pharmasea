#pragma once

#include <algorithm>

#include "../../ah.h"
#include "../../building_locations.h"
#include "../../camera.h"
#include "../../components/can_be_ghost_player.h"
#include "../../components/can_be_held.h"
#include "../../components/can_highlight_others.h"
#include "../../components/can_hold_furniture.h"
#include "../../components/can_hold_handtruck.h"
#include "../../components/can_hold_item.h"
#include "../../components/collects_user_input.h"
#include "../../components/has_base_speed.h"
#include "../../components/has_day_night_timer.h"
#include "../../components/has_fishing_game.h"
#include "../../components/has_work.h"
#include "../../components/is_bank.h"
#include "../../components/is_drink.h"
#include "../../components/is_free_in_store.h"
#include "../../components/is_item.h"
#include "../../components/is_item_container.h"
#include "../../components/is_rotatable.h"
#include "../../components/is_solid.h"
#include "../../components/is_store_spawned.h"
#include "../../components/is_trigger_area.h"
#include "../../components/transform.h"
#include "../../engine/assert.h"
#include "../../engine/log.h"
#include "../../engine/pathfinder.h"
#include "../../engine/runtime_globals.h"
#include "../../entities/entity.h"
#include "../../entities/entity_helper.h"
#include "../../entities/entity_id.h"
#include "../../entities/entity_query.h"
#include "../../entities/entity_type.h"
#include "../../network/server.h"
#include "../../system/core/system_manager.h"
#include "expected.hpp"
#include "is_collidable.h"

namespace system_manager {

void person_update_given_new_pos(
    int id, Transform& transform,
    // this could be const but is_collidable has no way to convert
    // between const entity& and OptEntity
    Entity& person, float, vec3 new_pos_x, vec3 new_pos_z);

namespace input_process_manager {

void collect_user_input(Entity& entity, float dt);
void process_player_movement_input(Entity& entity, float dt,
                                   float cam_angle_deg, InputName input_name,
                                   float input_amount);
void work_furniture(Entity& player, float frame_dt);
void fishing_game(Entity& player, float frame_dt);
void process_input(Entity& entity, const UserInputSnapshot& input);

}  // namespace input_process_manager

namespace planning {

void register_input_systems(afterhours::SystemManager& systems);
void rotate_furniture(const Entity& player);
void drop_held_furniture(Entity& player);
void handle_grab_or_drop(Entity& player);

}  // namespace planning

namespace inround {

void register_input_systems(afterhours::SystemManager& systems);
void handle_drop(Entity& player);
void handle_grab(Entity& player);
bool handle_drop_hand_truck(Entity& player);
bool handle_hand_truck(Entity& player);
void handle_grab_or_drop(Entity& player);

}  // namespace inround

}  // namespace system_manager