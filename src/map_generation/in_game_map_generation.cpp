#include "in_game_map_generation.h"

#include <string>
#include <vector>

#include "../ah.h"
#include "../building_locations.h"
#include "../entity_helper.h"
#include "map_generation.h"
#include "pipeline.h"

namespace mapgen {

vec2 generate_default_seed(const std::vector<std::string>& example_map) {
    generation::helper helper(example_map);
    // Default seed also uses BAR_BUILDING offset for consistency
    helper.set_world_offset(BAR_BUILDING.center());
    helper.generate();
    // Entities created during generation land in Afterhours' temp list; merge
    // before validation (and any EQ()/EntityQuery usage) so queries can see
    // required entities like Sophie/Register/etc.
    EntityHelper::get_current_collection().merge_entity_arrays();
    helper.validate();
    EntityHelper::invalidateCaches();
    // Keep behavior consistent with prior code: outside triggers were placed at
    // {0,0} for the default seed.
    return {0, 0};
}

vec2 generate_in_game_map(const std::string& seed) {
    GenerationContext ctx;
    GeneratedAscii ascii = generate_ascii(seed, ctx);

    generation::helper helper(ascii.lines);
    // Offset entities to be centered inside BAR_BUILDING
    helper.set_world_offset(BAR_BUILDING.center());
    vec2 max_location = helper.generate();
    // Same as default seed: merge temp entities before validation.
    EntityHelper::get_current_collection().merge_entity_arrays();
    helper.validate();
    EntityHelper::invalidateCaches();

    return max_location;
}

}  // namespace mapgen
