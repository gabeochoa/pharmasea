#pragma once

#include <optional>

#include "entity_helper.h"
#include "entity_id.h"

// A "persistence-friendly" entity reference:
// - On disk / over network: stores only EntityID (stable within a snapshot).
// - At runtime: optionally caches an EntityHandle for churn safety and O(1)
//   resolution when available.
//
// This is intentionally compatible with existing ID-based serialization: the
// serialized representation is just a 4-byte EntityID.
struct PersistentEntityRef {
    EntityID id = entity_id::INVALID;
    std::optional<afterhours::EntityHandle> handle{};

    [[nodiscard]] bool has_value() const { return id != entity_id::INVALID; }
    [[nodiscard]] bool empty() const { return !has_value(); }
    void clear() {
        id = entity_id::INVALID;
        handle.reset();
    }

    void set_id(EntityID new_id) {
        id = new_id;
        handle.reset();
    }

    void set(Entity& e) {
        id = e.id;
        // Handle will be invalid until the entity is merged/assigned a slot.
        const afterhours::EntityHandle h = EntityHelper::handle_for(e);
        if (h.valid()) {
            handle = h;
        } else {
            handle.reset();
        }
    }

    [[nodiscard]] OptEntity resolve() const {
        if (handle.has_value()) {
            OptEntity opt = EntityHelper::resolve(*handle);
            if (opt) return opt;
        }
        // Fallback preserves legacy behavior (including temp-entity scans via
        // PharmaSea's EntityHelper::getEntityForID wrapper).
        return EntityHelper::getEntityForID(id);
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        // Keep persistence ID-based for now (4 bytes), even if we cache handles
        // at runtime.
        s.value4b(id);
    }
};

