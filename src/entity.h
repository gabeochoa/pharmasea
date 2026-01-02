
#pragma once

#include "log/log.h"
//

#include "ah.h"
using afterhours::Entity;
using afterhours::EntityHandle;
using afterhours::OptEntity;
using afterhours::RefEntity;

#include "bitsery/ext/std_bitset.h"
#include "entity_type.h"
#include <bitsery/ext/std_map.h>

#include <optional>

// TODO memory? we could keep track of deleted entities and reuse those ids if
// we wanted to we'd have to be pretty disiplined about clearing people who
// store ids...

struct DebugOptions {
    EntityType type = EntityType::Unknown;
    bool enableCharacterModel = true;
};

using Item = Entity;

namespace bitsery {

using bitsery::ext::PointerObserver;
using bitsery::ext::PointerOwner;
using bitsery::ext::PointerType;
using bitsery::ext::StdBitset;
using bitsery::ext::StdMap;
using bitsery::ext::StdOptional;

template<typename S>
void serialize(S& s, EntityHandle& handle) {
    s.value4b(handle.slot);
    s.value4b(handle.gen);
}

template<typename S>
void serialize(S& s, const EntityHandle& handle) {
    s.value4b(handle.slot);
    s.value4b(handle.gen);
}

template<typename S>
void serialize(S& s, std::optional<EntityHandle>& opt_handle) {
    s.ext(opt_handle, StdOptional{},
          [](S& sv, EntityHandle& h) { serialize(sv, h); });
}

template<typename S>
void serialize(S& s, RefEntity ref) {
    Entity& e = ref.get();
    EntityHandle handle = afterhours::EntityHelper::handle_for(e);
    serialize(s, handle);
}

template<typename S>
void serialize(S& s, OptEntity opt) {
    std::optional<EntityHandle> opt_handle;
    if (opt.has_value()) {
        opt_handle = afterhours::EntityHelper::handle_for(opt.asE());
    }
    serialize(s, opt_handle);
}
}  // namespace bitsery

bool check_type(const Entity& entity, EntityType type);
bool check_if_drink(const Entity& entity);
EntityType get_entity_type(const Entity& entity);
