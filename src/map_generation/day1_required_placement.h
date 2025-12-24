#pragma once

#include <random>
#include <string>
#include <vector>

namespace mapgen {

// Places the minimum Day-1 required entities into an existing layout.
// Returns false if placement fails and the caller should reroll.
[[nodiscard]] bool place_required_day1(std::vector<std::string>& lines,
                                       std::mt19937& rng);

}  // namespace mapgen

