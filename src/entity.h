
#pragma once

#include "log/log.h"
//

#include "ah.h"
using afterhours::Entity;
using afterhours::OptEntity;
using afterhours::RefEntity;

#include "bitsery/ext/std_bitset.h"
#include "bitsery/ext/std_smart_ptr.h"
#include "entity_type.h"
//
#include <bitsery/ext/std_map.h>

// TODO memory? we could keep track of deleted entities and reuse those ids if
// we wanted to we'd have to be pretty disiplined about clearing people who
// store ids...

struct DebugOptions {
    EntityType type = EntityType::Unknown;
    bool enableCharacterModel = true;
};

using Item = Entity;

namespace bitsery {

using bitsery::ext::StdBitset;
using bitsery::ext::StdMap;
using bitsery::ext::StdSmartPtr;

template<typename S>
void serialize(S& s, Entity& entity) {
    s.value4b(entity.id);
    s.value4b(entity.entity_type);

    s.ext(entity.componentSet, StdBitset{});
    s.ext(entity.tags, StdBitset{});
    s.value1b(entity.cleanup);

    for (size_t i = 0; i < afterhours::max_num_components; ++i) {
        if (entity.componentSet.test(i)) {
            auto& ptr = entity.componentArray[i];
            s.ext(ptr, StdSmartPtr{});
        }
    }
}

template<typename S>
void serialize(S& s, std::shared_ptr<Entity>& entity) {
    // Pointer-free encoding: entities are serialized by value.
    //
    // NOTE: This does not preserve aliasing across multiple shared_ptrs to the
    // same Entity, but our game state should not rely on that. The goal is to
    // avoid serializing pointer identity/address data.
    if (!entity) {
        entity = std::make_shared<Entity>();
    }
    s.object(*entity);
}

template<typename S>
void serialize(S&, RefEntity) = delete;

template<typename S>
void serialize(S&, OptEntity) = delete;
}  // namespace bitsery

bool check_type(const Entity& entity, EntityType type);
bool check_if_drink(const Entity& entity);
EntityType get_entity_type(const Entity& entity);
