#pragma once

#include <string>
#include <vector>

#include "pipeline.h"

namespace mapgen {

[[nodiscard]] std::vector<std::string> generate_layout_wfc(
    const std::string& seed, const GenerationContext& ctx, int attempt_index);

}  // namespace mapgen

