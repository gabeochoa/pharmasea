
#pragma once

#include "../engine/is_server.h"
#include "../entity.h"

namespace system_manager {
namespace logging_manager {
void announce(std::shared_ptr<Entity> entity, const std::string& text) {
    // TODO have some way of distinguishing between server logs and regular
    // client logs
    if (is_server()) {
        log_info("server: {}({}): {}", entity->id,
                 entity->get<Transform>().pos(), text);
    } else {
        // log_info("client: {}: {}", this->id, text);
    }
}
}  // namespace logging_manager
}  // namespace system_manager
