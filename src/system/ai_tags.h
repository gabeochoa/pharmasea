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

    // Reserved slots (keep this range owned by AI).
    AIReserved_52 = 52,
    AIReserved_53 = 53,
    AIReserved_54 = 54,
    AIReserved_55 = 55,
    AIReserved_56 = 56,
    AIReserved_57 = 57,
    AIReserved_58 = 58,
    AIReserved_59 = 59,
};

}  // namespace afterhours::tags

