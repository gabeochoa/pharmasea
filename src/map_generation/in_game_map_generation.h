#pragma once

#include <string>
#include <vector>

#include "../ah.h"
#include "external_include.h"

namespace mapgen {

// Generates the "default_seed" map from the provided ASCII lines.
// Side-effect: spawns entities into the world (via `generation::helper`).
//
// Returns the origin/offset used by callers for placing "outside" triggers.
vec2 generate_default_seed(const std::vector<std::string>& example_map);

// Generates a non-default in-game map for the given seed.
// Side-effect: spawns entities into the world.
//
// Returns the origin/offset used by callers for placing "outside" triggers.
vec2 generate_in_game_map(const std::string& seed);

}  // namespace mapgen
