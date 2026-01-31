
#pragma once

#include "log/log.h"
//

#include "ah.h"
#include "zpp_bits_include.h"
using afterhours::Entity;
using afterhours::EntityHandle;
using afterhours::EntityID;
using afterhours::OptEntity;
using afterhours::RefEntity;

#include <optional>

#include "entity_type.h"

// TODO memory? we could keep track of deleted entities and reuse those ids if
// we wanted to we'd have to be pretty disiplined about clearing people who
// store ids...

struct DebugOptions {
    EntityType type = EntityType::Unknown;
    bool enableCharacterModel = true;
};

using Item = Entity;

namespace afterhours {
inline auto serialize(auto& archive, EntityHandle& handle) {
    return archive(   //
        handle.slot,  //
        handle.gen    //
    );
}

inline auto serialize(auto& archive, const EntityHandle& handle) {
    return archive(   //
        handle.slot,  //
        handle.gen    //
    );
}

inline auto serialize(auto& archive, std::optional<EntityHandle>& opt_handle) {
    return archive(  //
        opt_handle   //
    );
}

inline auto serialize(auto& archive, RefEntity ref) {
    Entity& e = ref.get();
    EntityHandle handle = afterhours::EntityHelper::handle_for(e);
    return archive(  //
        handle       //
    );
}

inline auto serialize(auto& archive, OptEntity opt) {
    std::optional<EntityHandle> opt_handle;
    if (opt.has_value()) {
        opt_handle = afterhours::EntityHelper::handle_for(opt.asE());
    }
    return archive(  //
        opt_handle   //
    );
}
}  // namespace afterhours

bool check_type(const Entity& entity, EntityType type);
bool check_if_drink(const Entity& entity);
EntityType get_entity_type(const Entity& entity);
