

#pragma once

#include <functional>
#include "../entity_helper.h"
#include "base_component.h"

using EntityID = int;

struct HasRopeToItem : public BaseComponent {
    virtual ~HasRopeToItem() {}

    void clear() {
        for (EntityID id : rope) {
            EntityHelper::markIDForCleanup(id);
        }
        rope.clear();
        rope_length = (int) rope.size();
        generated = false;
    }

    void mark_generated(vec2 p) {
        path_to = p;
        generated = true;
    }
    [[nodiscard]] bool was_generated() const { return generated; }

    void add(Item& i) {
        rope.push_back(i.id);
        rope_length = (int) rope.size();
    }

    [[nodiscard]] vec2 goal() const { return path_to; }

    // Snapshot support: remap stored EntityID references.
    void remap_entity_ids(const std::function<EntityID(EntityID)>& remap) {
        for (auto& id : rope) {
            id = remap(id);
        }
        rope_length = (int)rope.size();
    }

   private:
    vec2 path_to;
    bool generated = false;
    int rope_length = 0;
    std::vector<EntityID> rope;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.object(path_to);
        s.value1b(generated);
        s.value4b(rope_length);

        s.container4b(rope, rope_length);
    }
};
