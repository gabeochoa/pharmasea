#pragma once

#include <string>
#include <vector>

#include "pipeline.h"

namespace mapgen {

[[nodiscard]] std::vector<std::string> generate_layout_simple(
    const std::string& seed, const GenerationContext& ctx,
    BarArchetype archetype, int attempt_index);

}  // namespace mapgen
