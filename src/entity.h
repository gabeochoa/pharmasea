
#pragma once

#include "log/log.h"
//

#include "ah.h"
using afterhours::Entity;
using afterhours::EntityHandle;
using afterhours::OptEntity;
using afterhours::RefEntity;

#include "bitsery/ext/std_bitset.h"
#include "bitsery/ext/std_smart_ptr.h"
#include "entity_type.h"
//
#include <bitsery/ext/pointer.h>
#include <bitsery/ext/std_map.h>

#include <cstdint>
#include <optional>
#include "entity_handle_resolver.h"

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
using bitsery::ext::StdSmartPtr;

template<typename S>
void serialize(S& s, Entity& entity) {
    s.value4b(entity.id);
    s.value4b(entity.entity_type);
    s.ext(entity.componentSet, StdBitset{});
    s.ext(entity.tags, StdBitset{});
    s.value1b(entity.cleanup);
}

template<typename S>
void serialize(S& s, std::shared_ptr<Entity>& entity) {
    s.ext(entity, StdSmartPtr{});
}

template<typename S>
void serialize(S& s, EntityHandle& handle) {
    // Wire format: (u32 slot, u32 gen). Keep stable across platforms.
    uint32_t slot32 = static_cast<uint32_t>(handle.slot);
    uint32_t gen32 = static_cast<uint32_t>(handle.gen);
    s.value4b(slot32);
    s.value4b(gen32);
    handle.slot = static_cast<afterhours::EntityHandle::Slot>(slot32);
    handle.gen = static_cast<std::size_t>(gen32);
}

template<typename S>
void serialize(S& s, const EntityHandle& handle) {
    EntityHandle tmp = handle;
    serialize(s, tmp);
}

template<typename S>
void serialize(S& s, std::optional<EntityHandle>& opt_handle) {
    s.ext(opt_handle, StdOptional{},
          [](S& sv, EntityHandle& h) { serialize(sv, h); });
}

template<typename S>
void serialize(S& s, RefEntity ref) {
    Entity& e = ref.get();
    EntityHandle handle = pharmasea_handles::handle_for(e);
    serialize(s, handle);
}

template<typename S>
void serialize(S& s, OptEntity opt) {
    std::optional<EntityHandle> opt_handle;
    if (opt.has_value()) {
        opt_handle = pharmasea_handles::handle_for(opt.asE());
    }
    serialize(s, opt_handle);
}
}  // namespace bitsery

bool check_type(const Entity& entity, EntityType type);
bool check_if_drink(const Entity& entity);
EntityType get_entity_type(const Entity& entity);
