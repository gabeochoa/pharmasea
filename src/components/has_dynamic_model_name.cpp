

#include "has_dynamic_model_name.h"

#include "../system/system_manager.h"

std::string HasDynamicModelName::fetch(const Entity& owner) const {
    if (!initialized)
        log_warn("calling HasDynamicModelName::fetch() without initializing");

    switch (dynamic_type) {
        case OpenClosed: {
            bool is_daytime = SystemManager::get().is_daytime();
            if (!is_daytime) return base_name;
            return fmt::format("open_{}", base_name);
        } break;
            // TODO eventually id like the logic to live in here assuming we
            // have a ton using these. if its just one for each then fetcher
            // (Custom:) is perfectly fine
        case NoDynamicType:
        case Ingredients:
        case EmptyFull:
        case Subtype: {
            return fetcher(owner, base_name);
        } break;
    }
    return base_name;
}
