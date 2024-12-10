
#pragma once

#include "afterhours/ah.h"
using afterhours::Entity;

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

using RefEntity = std::reference_wrapper<Entity>;
using OptEntityType = std::optional<std::reference_wrapper<Entity>>;

struct OptEntity {
    OptEntityType data;

    OptEntity() {}
    OptEntity(OptEntityType opt_e) : data(opt_e) {}
    OptEntity(RefEntity _e) : data(_e) {}
    OptEntity(Entity& _e) : data(_e) {}

    bool has_value() const { return data.has_value(); }
    bool valid() const { return has_value(); }

    Entity* value() { return &(data.value().get()); }
    Entity* operator*() { return value(); }
    Entity* operator->() { return value(); }

    const Entity* value() const { return &(data.value().get()); }
    const Entity* operator*() const { return value(); }
    // TODO look into switching this to using functional monadic stuff
    // i think optional.transform can take and handle calling functions on an
    // optional without having to expose the underlying value or existance of
    // value
    const Entity* operator->() const { return value(); }

    Entity& asE() { return data.value(); }
    const Entity& asE() const { return data.value(); }

    operator RefEntity() { return data.value(); }
    operator RefEntity() const { return data.value(); }
    operator bool() const { return valid(); }
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
