#pragma once

#include "ah.h"

// Thin bridge so `src/entity.h` can serialize RefEntity/OptEntity via handles
// without including `entity_helper.h` (avoids include cycles).
namespace pharmasea_handles {

// Returns a stable handle for a live entity, or EntityHandle::invalid().
afterhours::EntityHandle handle_for(const afterhours::Entity& e);

// Resolves a handle to a live entity (or empty).
afterhours::OptEntity resolve(afterhours::EntityHandle h);

}  // namespace pharmasea_handles

