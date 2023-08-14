

#pragma once

#include <string>  // for basic_string

#include "components/has_work.h"
#include "engine/ui_color.h"
#include "entity.h"   // for Item, DebugOptions, Entity
#include "raylib.h"   // for Color, PINK
#include "strings.h"  // for GRABBER, CONVEYER, FILTERED_GRABBER

typedef Entity Furniture;

void convert_to_type(EntityType& entity_type, Entity& entity, vec2 location);

void register_all_components();
void make_remote_player(Entity& remote_player, vec3 pos);
void update_player_remotely(Entity& entity, float* location,
                            const std::string& username, int facing_direction);
void update_player_rare_remotely(Entity& entity, int model_index,
                                 long long last_ping);
void make_player(Entity& player, vec3 p);

namespace furniture {

void make_wall(Entity& wall, vec2 pos, Color c = ui::color::brown);

void make_character_switcher(Entity& character_switcher, vec2 pos);
void make_map_randomizer(Entity&, vec2 pos);
void make_fast_forward(Entity&, vec2 pos);
void make_trigger_area(Entity& trigger_area, vec3 pos, float width,
                       float height);

}  // namespace furniture

namespace items {
void make_item_type(Item& item, EntityType type_name, vec2 pos, int index = -1);

// Returns true if item was cleaned up
bool _add_ingredient_to_drink_NO_VALIDATION(Entity& drink, Item& toadd);

}  // namespace items
