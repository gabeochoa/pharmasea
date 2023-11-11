
#pragma once

#include "../components/debug_name.h"
#include "../components/transform.h"
#include "../engine/is_server.h"
#include "../entity.h"

namespace system_manager {
namespace logging_manager {
inline void announce(const Entity& entity, const std::string& text) {
    // TODO have some way of distinguishing between server logs and regular
    // client logs
    if (is_server()) {
        log_trace("server: {}({})@{}: {}", entity.get<DebugName>().name(),
                  entity.id, entity.get<Transform>().pos(), text);
    } else {
        // log_info("client: {}: {}", this->id, text);
    }
}
}  // namespace logging_manager
}  // namespace system_manager
