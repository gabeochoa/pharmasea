

#pragma once

#include "components/has_rope_to_item.h"
#include "components/has_subtype.h"
#include "components/is_pnumatic_pipe.h"
#include "components/is_progression_manager.h"
#include "dataclass/ingredient.h"
#include "entity.h"
//
#include "components/adds_ingredient.h"
#include "components/can_be_ghost_player.h"
#include "components/can_be_held.h"
#include "components/can_be_highlighted.h"
#include "components/can_be_pushed.h"
#include "components/can_be_taken_from.h"
#include "components/can_grab_from_other_furniture.h"
#include "components/can_highlight_others.h"
#include "components/can_hold_furniture.h"
#include "components/can_hold_item.h"
#include "components/can_order_drink.h"
#include "components/can_perform_job.h"
#include "components/collects_user_input.h"
#include "components/conveys_held_item.h"
#include "components/custom_item_position.h"
#include "components/has_base_speed.h"
#include "components/has_client_id.h"
#include "components/has_dynamic_model_name.h"
#include "components/has_name.h"
#include "components/has_rope_to_item.h"
#include "components/has_speech_bubble.h"
#include "components/has_timer.h"
#include "components/has_waiting_queue.h"
#include "components/has_work.h"
#include "components/indexer.h"
#include "components/is_drink.h"
#include "components/is_item_container.h"
#include "components/is_pnumatic_pipe.h"
#include "components/is_progression_manager.h"
#include "components/is_rotatable.h"
#include "components/is_snappable.h"
#include "components/is_solid.h"
#include "components/is_spawner.h"
#include "components/is_trigger_area.h"
#include "components/model_renderer.h"
#include "components/responds_to_user_input.h"
#include "components/shows_progress_bar.h"
#include "components/simple_colored_box_renderer.h"
#include "components/transform.h"
#include "components/uses_character_model.h"
#include "recipe_library.h"
#include "strings.h"

typedef Entity Furniture;

void register_all_components();
void add_entity_components(Entity& entity);
void add_person_components(Entity& person);
void make_entity(Entity& entity, const DebugOptions& options,
                 vec3 p = {-2, -2, -2});
void add_player_components(Entity& player);

void make_remote_player(Entity& remote_player, vec3 pos);

void move_player_SERVER_ONLY(Entity& entity, vec3 position);

void update_player_remotely(Entity& entity, float* location,
                            const std::string& username, int facing_direction);

void update_player_rare_remotely(Entity& entity, int model_index,
                                 long long last_ping);

void make_player(Entity& player, vec3 p);
void make_aiperson(Entity& person, const DebugOptions& options, vec3 p);

namespace furniture {
void make_furniture(Entity& furniture, const DebugOptions& options, vec2 pos,
                    Color face = PINK, Color base = PINK,
                    bool is_static = false);

void process_table_working(Entity& table, HasWork& hasWork, Entity& player,
                           float dt);

void make_table(Entity& table, vec2 pos);

void make_character_switcher(Entity& character_switcher, vec2 pos);

void make_wall(Entity& wall, vec2 pos, Color c = ui::color::brown);

void make_conveyer(Entity& conveyer, vec2 pos,
                   const DebugOptions& options = {
                       .name = strings::entity::CONVEYER});

void make_grabber(Entity& grabber, vec2 pos,
                  const DebugOptions& options = {
                      .name = strings::entity::GRABBER});

void make_filtered_grabber(Entity& grabber, vec2 pos,
                           const DebugOptions& options = DebugOptions{
                               .name = strings::entity::FILTERED_GRABBER});

void make_register(Entity& reg, vec2 pos);

void make_itemcontainer(Entity& container, const DebugOptions& options,
                        vec2 pos, const std::string& item_type);

void make_squirter(Entity& squ, vec2 pos);

void make_trash(Entity& trash, vec2 pos);

void make_pnumatic_pipe(Entity& pnumatic, vec2 pos);

void make_medicine_cabinet(Entity& container, vec2 pos);

void make_fruit_basket(Entity& container, vec2 pos);

void make_cupboard(Entity& cupboard, vec2 pos);

void make_soda_machine(Entity& soda_machine, vec2 pos);

void make_mop_holder(Entity& mop_holder, vec2 pos);

void make_trigger_area(
    Entity& trigger_area, vec3 pos, float width, float height,
    const std::string& title = strings::entity::TRIGGER_AREA);
void make_blender(Entity& blender, vec2 pos);

// This will be a catch all for anything that just needs to get updated
void make_sophie(Entity& sophie, vec3 pos);

void make_vomit(Entity& vomit, vec2 pos);

}  // namespace furniture

namespace items {

void make_item(Item& item, const DebugOptions& options, vec2 p = {0, 0});

void make_soda_spout(Item& soda_spout, vec2 pos);

void make_mop(Item& mop, vec2 pos);

// Returns true if item was cleaned up
bool _add_ingredient_to_drink_NO_VALIDATION(Entity& drink, Item& toadd);

void process_drink_working(Entity& drink, HasWork& hasWork, Entity& player,
                           float dt);

void make_alcohol(Item& alc, vec2 pos, int index);

void make_simple_syrup(Item& simple_syrup, vec2 pos);

void make_lemon(Item& lemon, vec2 pos, int index);

void make_drink(Item& drink, vec2 pos);

void make_item_type(Item& item, const std::string& type_name,  //
                    vec2 pos,                                  //
                    int index = -1                             //
);

}  // namespace items

void make_customer(Entity& customer, vec2 p, bool has_order = true);

namespace furniture {
void make_customer_spawner(Entity& customer_spawner, vec3 pos);

}  // namespace furniture
