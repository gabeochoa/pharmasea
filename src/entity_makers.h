

#pragma once

#include <string>  // for basic_string

#include "components/has_work.h"
#include "components/is_floor_marker.h"
#include "components/is_trigger_area.h"
#include "engine/ui/color.h"
#include "entity.h"   // for Item, DebugOptions, Entity
#include "raylib.h"   // for Color, PINK
#include "strings.h"  // for GRABBER, CONVEYER, FILTERED_GRABBER

typedef Entity Furniture;

void convert_to_type(const EntityType& entity_type, Entity& entity,
                     vec2 location);

void register_all_components();
void make_remote_player(Entity& remote_player, vec3 pos);
void update_player_remotely(Entity& entity, float* location,
                            const std::string& username, float facing);
void update_player_rare_remotely(Entity& entity, int model_index,
                                 long long last_ping);
void make_player(Entity& player, vec3 p);

namespace furniture {

void make_wall(Entity& wall, vec2 pos, Color c = ui::color::brown);

void make_character_switcher(Entity& character_switcher, vec2 pos);
void make_map_randomizer(Entity&, vec2 pos);
void make_fast_forward(Entity&, vec2 pos);
// TODO :BE: i dont like that we need to include ITA here...
void make_trigger_area(Entity& trigger_area, vec3 pos, float width,
                       float height, IsTriggerArea::Type type);
void make_floor_marker(Entity& floor_marker, vec3 pos, float width,
                       float height, IsFloorMarker::Type type);

}  // namespace furniture

namespace items {
void make_item_type(Item& item, EntityType type_name, vec2 pos, int index = -1);

// Returns true if item was cleaned up
bool _add_ingredient_to_drink_NO_VALIDATION(Entity& drink, Item& toadd);

}  // namespace items
