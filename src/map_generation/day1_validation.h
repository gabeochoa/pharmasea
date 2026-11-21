#pragma once

#include <string>
#include <vector>

namespace mapgen {

// Shared Day-1 validation (ASCII + basic routing parity).
[[nodiscard]] bool validate_day1_ascii_plus_routing(
    const std::vector<std::string>& lines);

}  // namespace mapgen
