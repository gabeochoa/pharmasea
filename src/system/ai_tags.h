#pragma once

#include "../ah.h"
#include "../entity_type.h"

// Runtime tag IDs for gameplay and AI flow control.
//
// Note: afterhours tags are a fixed-size bitset (default 64). We reserve high
// IDs to avoid colliding with EntityType tags, which occupy low indices.
//
// We group IDs by 10s for readability where possible. EntityType currently
// occupies tag IDs [0, enum_count(EntityType)-1]. Gameplay tags live in the
// 56–59 range, and AI tags live in the 60–63 range.
namespace afterhours::tags {

enum class TriggerTag : afterhours::TagId {
    GateTriggerWhileOccupied = 56,
    TriggerFiredWhileOccupied = 57,
    TriggerAreaFullNeedsProcessing = 58,
    TriggerAreaHasDynamicText = 59,
};

enum class AITag : afterhours::TagId {
    AINeedsResetting = 60,
    AITransitionPending = 61,

    // Reserved slots (keep this range owned by AI).
    AIReserved_62 = 62,
    AIReserved_63 = 63,
};

}  // namespace afterhours::tags

static_assert(magic_enum::enum_count<EntityType>() <= 56,
              "EntityType tags must not collide with AI tag IDs");
