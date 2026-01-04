#pragma once

#include "../ah.h"

// Runtime tag IDs for AI flow control.
//
// Note: afterhours tags are a fixed-size bitset (default 64). We reserve high
// IDs to avoid colliding with EntityType tags, which occupy low indices.
namespace afterhours::tags {

enum class AITag : afterhours::TagId {
    AINeedsResetting = 62,
    AITransitionPending = 63,
};

}  // namespace afterhours::tags

