#pragma once

#include "../ah.h"
#include "../entity_type.h"

#include "magic_enum/magic_enum.hpp"

// Project-reserved afterhours tag IDs (distinct from EntityType tags).
//
// Convention: tag IDs in [0, enum_count<EntityType>()) are reserved for the
// EntityType bit (one per entity). Tags at/after enum_count<EntityType>() are
// available for gameplay/system flags.
namespace afterhours::tags {

struct AITag {
    // Tag indicating an AI transition has been staged this frame.
    static constexpr afterhours::TagId AITransitionPending =
        static_cast<afterhours::TagId>(magic_enum::enum_count<EntityType>() + 0);

    // Tag indicating the AI entity should run its "on-enter" reset logic.
    static constexpr afterhours::TagId AINeedsResetting =
        static_cast<afterhours::TagId>(magic_enum::enum_count<EntityType>() + 1);
};

static_assert(static_cast<size_t>(AITag::AINeedsResetting) < 64,
              "Afterhours Entity tags bitset is expected to be 64 bits");

}  // namespace afterhours::tags

