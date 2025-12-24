#include "in_game_map_generation.h"

#include <string>
#include <vector>

#include "../ah.h"
#include "../entity_helper.h"
#include "map_generation.h"
#include "pipeline.h"

namespace mapgen {

vec2 generate_default_seed(const std::vector<std::string>& example_map) {
    generation::helper helper(example_map);
    helper.generate();
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
    vec2 max_location = helper.generate();
    helper.validate();
    EntityHelper::invalidateCaches();

    return max_location;
}

}  // namespace mapgen

