#pragma once

// PharmaSea glue for Afterhours "Collections" (multi-world ECS).
//
// Afterhours now requires a per-thread active `afterhours::EntityCollection`
// for ComponentStore/EntityHelper access. PharmaSea runs server + client in one
// process, so we maintain two collections and bind them per thread.

#include "ah.h"

namespace pharmasea_afterhours {

// Global ECS worlds for this process.
afterhours::EntityCollection& server_collection();
afterhours::EntityCollection& client_collection();

// Bind the desired collection for the current thread (idempotent).
void bind_server_collection_for_this_thread();
void bind_client_collection_for_this_thread();

}  // namespace pharmasea_afterhours

