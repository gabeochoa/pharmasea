#pragma once

#include "../ah.h"

// Runtime tag IDs for AI flow control.
//
// Note: afterhours tags are a fixed-size bitset (default 64). We reserve high
// IDs to avoid colliding with EntityType tags, which occupy low indices.
//
// We also group IDs by 10s for readability; AI tags live in the 50–59 range.
namespace afterhours::tags {

enum class AITag : afterhours::TagId {
    AINeedsResetting = 50,
    AITransitionPending = 51,
};

}  // namespace afterhours::tags

