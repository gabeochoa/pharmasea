#pragma once

#include "ah.h"
using afterhours::Entities;

namespace server_only {

void fix_all_container_item_types(Entities& entities);
void reinit_dynamic_model_names_after_load(Entities& entities);
void run_all_post_load_helpers(Entities& entities);
void run_all_post_load_helpers();

}  // namespace server_only
