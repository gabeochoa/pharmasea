#pragma once

#include <optional>

#include "entity.h"
#include "entity_id.h"

// A "persistence-friendly" entity reference:
// - On disk / over network: stores only EntityID (stable within a snapshot).
// - At runtime: optionally caches an EntityHandle for churn safety and O(1)
//   resolution when available.
//
// This is intentionally compatible with existing ID-based serialization: the
// serialized representation is just a 4-byte EntityID.
struct EntityRef {
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

    // Implemented in entity_ref.cpp to break circular dependency
    void set(Entity& e);
    [[nodiscard]] OptEntity resolve() const;
    // Like resolve(), but crashes if entity is missing. Use when entity must
    // exist.
    [[nodiscard]] Entity& resolve_enforced() const;

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        // Keep persistence ID-based for now (4 bytes), even if we cache handles
        // at runtime.
        return archive(  //
            self.id      //
        );
    }
};
