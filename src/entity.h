
#pragma once

#include "log/log.h"
//

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
    log_trace("Serializing Entity id:{} type:{}", entity.id,
              entity.entity_type);
    s.value4b(entity.id);
    s.value4b(entity.entity_type);

    s.ext(entity.componentSet, StdBitset{});
    s.value1b(entity.cleanup);

    for (size_t i = 0; i < afterhours::max_num_components; ++i) {
        if (entity.componentSet.test(i)) {
            auto& ptr = entity.componentArray[i];
            log_trace("  Component entry id:{} present:{}", i,
                      static_cast<bool>(ptr));
            if (ptr) {
                log_info("About to serialize component at index {}", i);
            }
            s.ext(ptr, StdSmartPtr{});
            if (ptr) {
                log_info("Successfully serialized component at index {}", i);
            }
        }
    }
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
