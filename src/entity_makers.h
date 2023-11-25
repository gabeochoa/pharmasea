

#pragma once

#include <string>  // for basic_string

#include "components/has_work.h"
#include "components/is_floor_marker.h"
#include "components/is_spawner.h"
#include "components/is_trigger_area.h"
#include "dataclass/ingredient.h"
#include "engine/ui/color.h"
#include "entity.h"   // for Item, DebugOptions, Entity
#include "raylib.h"   // for Color, PINK
#include "strings.h"  // for GRABBER, CONVEYER, FILTERED_GRABBER

typedef Entity Furniture;

bool convert_to_type(const EntityType& entity_type, Entity& entity,
                     vec2 location);

void register_all_components();
void make_remote_player(Entity& remote_player, vec3 pos);
void update_player_remotely(Entity& entity, float* location,
                            const std::string& username, float facing);
void update_player_rare_remotely(Entity& entity, int model_index,
                                 long long last_ping);
void make_player(Entity& player, vec3 p);
void make_customer(Entity& customer, const SpawnInfo& info, bool has_order);

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
void make_single_alcohol(Entity&, vec2, int);
void make_vomit(Entity&, const SpawnInfo&);
void make_fruit_basket(Entity&, vec2, int starting_index);
void make_table(Entity&, vec2);
void make_cupboard(Entity&, vec2, int index = 0);

}  // namespace furniture

namespace items {
void make_juice(Item& juice, vec2 pos, Ingredient fruit);
void make_drink(Item& drink, vec2 pos);

void make_item_type(Item& item, EntityType type_name, vec2 pos, int index = -1);

// Returns true if item was cleaned up
bool _add_item_to_drink_NO_VALIDATION(Entity& drink, Item& toadd);

}  // namespace items
