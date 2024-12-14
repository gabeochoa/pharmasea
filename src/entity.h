
#pragma once

#include "afterhours/ah.h"
using afterhours::Entity;
using afterhours::OptEntity;
using afterhours::RefEntity;

#include "bitsery/ext/std_bitset.h"
#include "bitsery/ext/std_smart_ptr.h"
#include "entity_type.h"
//
#include <bitsery/ext/pointer.h>
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
    s.value1b(entity.cleanup);

    s.ext(entity.componentArray, StdMap{afterhours::max_num_components},
          [](S& sv, afterhours::ComponentID& key,
             std::unique_ptr<afterhours::BaseComponent>(&value)) {
              sv.value8b(key);
              // sv.ext(value, PointerOwner{PointerType::Nullable});
              sv.ext(value, StdSmartPtr{});
          });
}

template<typename S>
void serialize(S& s, std::shared_ptr<Entity>& entity) {
    s.ext(entity, StdSmartPtr{});
}

template<typename S>
void serialize(S& s, RefEntity ref) {
    Entity& e = ref.get();
    Entity* eptr = &e;
    s.ext8b(eptr, PointerObserver{});
}

template<typename S>
void serialize(S& s, OptEntity opt) {
    s.ext(opt, StdOptional{});
}
}  // namespace bitsery

bool check_type(const Entity& entity, EntityType type);
bool check_if_drink(const Entity& entity);
