

#include "has_dynamic_model_name.h"

#include "../system/system_manager.h"

std::string HasDynamicModelName::fetch(const Entity& owner) const {
    if (!initialized)
        log_warn("calling HasDynamicModelName::fetch() without initializing");

    const auto fallback_base_name = [&]() -> std::string {
        if (!base_name.empty()) return base_name;
        return std::string(util::convertToSnakeCase<EntityType>(owner.entity_type));
    };

    switch (dynamic_type) {
        case OpenClosed: {
            bool is_daytime = SystemManager::get().is_daytime();
            std::string base = fallback_base_name();
            if (!is_daytime) return base;
            return fmt::format("open_{}", base);
        } break;
            // TODO eventually id like the logic to live in here assuming we
            // have a ton using these. if its just one for each then fetcher
            // (Custom:) is perfectly fine
        case NoDynamicType:
        case Ingredients:
        case EmptyFull:
        case Subtype: {
            if (!fetcher) {
                // This can happen for loaded save snapshots: the runtime
                // std::function isn't serialized. Fall back to a stable name
                // instead of crashing (std::bad_function_call).
                return fallback_base_name();
            }
            return fetcher(owner, fallback_base_name());
        } break;
    }
    return fallback_base_name();
}
