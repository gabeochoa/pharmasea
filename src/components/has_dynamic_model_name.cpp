

#include "has_dynamic_model_name.h"

#include "../system/core/system_manager.h"

std::string HasDynamicModelName::fetch(const Entity& owner) const {
    if (!initialized) {
        log_warn("calling HasDynamicModelName::fetch() without initializing");
        return base_name;  // Return default when not initialized
    }

    switch (dynamic_type) {
        case OpenClosed: {
            bool is_bar_open = SystemManager::get().is_bar_open();
            if (!is_bar_open) return base_name;
            return fmt::format("open_{}", base_name);
        } break;
            // TODO eventually id like the logic to live in here assuming we
            // have a ton using these. if its just one for each then fetcher
            // (Custom:) is perfectly fine
        case NoDynamicType:
        case Ingredients:
        case EmptyFull:
        case Subtype: {
            if (fetcher) {
                return fetcher(owner, base_name);
            } else {
                log_warn(
                    "HasDynamicModelName fetcher is null for dynamic_type {}, "
                    "returning base_name",
                    (int) dynamic_type);
                return base_name;
            }
        } break;
    }
    return base_name;
}
