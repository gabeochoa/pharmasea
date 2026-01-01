#pragma once

#include "entity.h"  // EntityHandle wire format + bitsery helpers

// Example component DTOs used by WorldSnapshotV2.
// Add additional DTOs as we wire capture/apply for more components.
#include "components/transform.h"

#include "bitsery/ext/std_bitset.h"
#include "bitsery/ext/std_tuple.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <vector>

// Pointer-free snapshot payload (V2)
//
// Goals:
// - Single serializable payload for save-game and network "map" transfers.
// - No smart pointers, no raw pointers, no PointerLinkingContext required.
// - Entity identity keyed by Afterhours EntityHandle (slot + generation).
// - Component data stored as value lists keyed by EntityHandle.
//
// Notes:
// - During transition we keep `legacy_id` for debugging/back-compat checks, but
//   handles are the canonical key.
// - Components should snapshot DTOs ("projectors") when the live component type
//   is non-copyable or contains pointer-like fields.
namespace snapshot_v2 {

inline constexpr std::uint32_t kWorldSnapshotVersionV2 = 2;

// Safety bounds for Bitsery containers. Adjust if/when we support very large maps.
inline constexpr std::size_t kMaxSnapshotEntities = 250'000;
inline constexpr std::size_t kMaxSnapshotComponentsPerType = 250'000;
inline constexpr std::size_t kMaxSnapshotEntityComponentBytes = 4 * 1024 * 1024;  // 4 MiB per entity
inline constexpr std::size_t kMaxSnapshotTotalComponentBytes =
    128ULL * 1024ULL * 1024ULL;  // 128 MiB total arena

struct EntityRecordV2 {
    afterhours::EntityHandle handle{};

    // Transitional/debug: the runtime EntityID value at capture time.
    // Do not rely on this for stable identity.
    // NOTE: avoid depending on a particular afterhours invalid-ID constant name.
    // Afterhours uses -1 as the invalid EntityID sentinel.
    afterhours::EntityID legacy_id = afterhours::EntityID{-1};

    int entity_type = 0;
    afterhours::ComponentBitSet component_set{};
    afterhours::TagBitset tags{};
    bool cleanup = false;

    // Component payload slice into WorldSnapshotV2::component_bytes.
    // This keeps network serialization single-pass-friendly and avoids
    // per-entity heap allocations.
    std::uint32_t components_offset = 0;
    std::uint32_t components_size = 0;
};

// Generic per-component snapshot list, keyed by EntityHandle.
template<typename V>
struct ComponentListV2 {
    static_assert(!std::is_pointer_v<V>, "snapshot value must be pointer-free");

    struct Entry {
        afterhours::EntityHandle entity{};
        V value{};

        template<typename S>
        void serialize(S& s) {
            s.object(entity);
            s.object(value);
        }
    };

    std::vector<Entry> entries{};

    template<typename S>
    void serialize(S& s) {
        s.container(entries, snapshot_v2::kMaxSnapshotComponentsPerType);
    }
};

// Example DTO for Transform (live Transform is non-copyable via BaseComponent).
struct TransformV2 {
    vec3 visual_offset{};
    vec3 raw_position{};
    vec3 position{};
    float facing = 0.f;
    vec3 size{TILESIZE, TILESIZE, TILESIZE};
};

struct WorldSnapshotV2 {
    std::uint32_t version = kWorldSnapshotVersionV2;

    std::vector<EntityRecordV2> entities{};
    std::vector<std::uint8_t> component_bytes{};

    struct Components {
        ComponentListV2<TransformV2> transform{};
        // TODO(V2): add additional component DTO lists here as we wire capture/apply.
    } components{};
};

}  // namespace snapshot_v2

namespace bitsery {
using bitsery::ext::StdBitset;
using bitsery::ext::StdTuple;

template<typename S>
void serialize(S& s, snapshot_v2::EntityRecordV2& e) {
    s.object(e.handle);
    s.value4b(e.legacy_id);
    s.value4b(e.entity_type);
    s.ext(e.component_set, StdBitset{});
    s.ext(e.tags, StdBitset{});
    s.value1b(e.cleanup);
    s.value4b(e.components_offset);
    s.value4b(e.components_size);
}

template<typename S>
void serialize(S& s, snapshot_v2::TransformV2& t) {
    s.object(t.visual_offset);
    s.object(t.raw_position);
    s.object(t.position);
    s.value4b(t.facing);
    s.object(t.size);
}

template<typename S>
void serialize(S& s, snapshot_v2::WorldSnapshotV2::Components& c) {
    s.object(c.transform);
}

template<typename S>
void serialize(S& s, snapshot_v2::WorldSnapshotV2& snap) {
    s.value4b(snap.version);
    s.container(snap.entities, snapshot_v2::kMaxSnapshotEntities);
    // Serialize byte arena with a 4-byte length prefix.
    //
    // IMPORTANT: don't use bitsery's `container4b(std::vector<uint8_t>)` fast-path:
    // it assumes `sizeof(T) == 4` and will `static_assert` on uint8_t/unsigned char.
    //
    // We also cap the size to keep deserialization bounded, while still consuming
    // the full byte stream to remain aligned for subsequent fields.
    std::uint32_t n = static_cast<std::uint32_t>(snap.component_bytes.size());
    if (n > snapshot_v2::kMaxSnapshotTotalComponentBytes) {
        n = static_cast<std::uint32_t>(snapshot_v2::kMaxSnapshotTotalComponentBytes);
    }
    s.value4b(n);
    const std::uint32_t keep = n;
    snap.component_bytes.resize(keep);
    for (std::uint32_t i = 0; i < keep; ++i) {
        s.value1b(snap.component_bytes[i]);
    }
    s.object(snap.components);
}
}  // namespace bitsery

