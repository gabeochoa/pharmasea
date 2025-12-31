#include "entity_handle_resolver.h"

#include "entity_helper.h"

namespace pharmasea_handles {

afterhours::EntityHandle handle_for(const afterhours::Entity& e) {
    return EntityHelper::handle_for(e);
}

afterhours::OptEntity resolve(afterhours::EntityHandle h) {
    return EntityHelper::resolve(h);
}

}  // namespace pharmasea_handles

