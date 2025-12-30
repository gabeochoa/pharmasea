#pragma once

#include "ah.h"

// NOTE: Today `afterhours::EntityID` is an `int` alias, so it cannot have
// members like `EntityID::INVALID`. Use this constant instead.
namespace entity_id {
inline constexpr afterhours::EntityID INVALID = -1;
}

