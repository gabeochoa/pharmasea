#include "in_game_map_generation.h"

#include <iostream>
#include <string>
#include <vector>

#include "../ah.h"
#include "../entity_helper.h"
#include "../engine/random_engine.h"
#include "map_generation.h"
#include "simple.h"

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

vec2 generate_in_game_map(const std::string& /*seed*/) {
    // NOTE: This currently uses the "simple" room-expansion generator (see
    // `something()`), and then feeds the resulting ASCII into
    // `generation::helper` to instantiate entities.
    //
    // TODO: The WFC path is present but currently disabled/commented out.
    int rows = 20;
    int cols = 20;
    std::vector<char> chars = something(rows, cols);

    std::vector<std::string> lines;
    std::string tmp;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            tmp.push_back(chars[i * rows + j]);
        }
        lines.push_back(tmp);
        tmp.clear();
    }

    // NOTE: This “required entities strip” is a debug/placeholder behavior and
    // is not spec-compliant long-term (it creates disconnected regions and does
    // not ensure runtime constraints like register-inside-building or queue
    // strip clearance).
    //
    // Phase-1 spec doc: `docs/map_playability_spec.md`
    std::vector<char> required = {{
        generation::CUST_SPAWNER,
        generation::SODA_MACHINE,
        generation::TRASH,
        generation::REGISTER,
        generation::ORIGIN,
        generation::SOPHIE,
        generation::FAST_FORWARD,
        generation::CUPBOARD,

        generation::TABLE,
        generation::TABLE,
        generation::TABLE,
        generation::TABLE,
    }};
    tmp.clear();
    for (auto c : required) {
        tmp.push_back(c);
        if (tmp.size() == (size_t) rows) {
            lines.push_back(tmp);
            tmp.clear();
        }
    }
    lines.push_back(tmp);

    for (const auto& c : lines) {
        std::cout << c;
        std::cout << std::endl;
    }

    generation::helper helper(lines);
    vec2 max_location = helper.generate();
    helper.validate();
    EntityHelper::invalidateCaches();

    log_info("max location {}", max_location);

    return max_location;
}

}  // namespace mapgen

