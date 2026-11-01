#pragma once

#include "../../entities/entity.h"
#include "../../entities/entity_type.h"
#include "../../vendor_include.h"

namespace system_manager {

namespace input_process_manager {

[[nodiscard]] bool is_collidable(const Entity& entity, OptEntity other = {});
[[nodiscard]] bool is_collidable(const Entity& entity, const Entity& other);

}  // namespace input_process_manager

}  // namespace system_manager