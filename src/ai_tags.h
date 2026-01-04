#pragma once

#include "ah.h"
#include "entity_type.h"

#include "magic_enum/magic_enum.hpp"

namespace afterhours::tags {

// Project-level tags that are not part of EntityType.
// These must use tag ids outside the EntityType enum range.
enum class AITag : afterhours::TagId {
    // Entity has requested an AI transition this frame; `IsAIControlled::next_state`
    // contains the staged destination.
    AITransitionPending =
        static_cast<afterhours::TagId>(magic_enum::enum_count<::EntityType>()),
};

[[nodiscard]] inline constexpr afterhours::TagId tag_id(const AITag t) {
    return static_cast<afterhours::TagId>(t);
}

}  // namespace afterhours::tags

