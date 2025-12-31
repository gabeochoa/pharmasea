#include "world_snapshot_v2_runtime.h"

#include "ah.h"
#include "engine/log.h"
#include "world_snapshot_v2_components_blob.h"

#include <unordered_map>

// ComponentStore is present in newer Afterhours versions (pooled components).
// Some dev environments may have an older submodule checkout; keep this file
// compiling by feature-detecting the header.
#if __has_include("afterhours/src/core/component_store.h")
#include "afterhours/src/core/component_store.h"
#define PHARMASEA_HAS_AFTERHOURS_COMPONENT_STORE 1
#else
#define PHARMASEA_HAS_AFTERHOURS_COMPONENT_STORE 0
#endif

namespace snapshot_v2 {
namespace {

struct EntityHandleHash {
    std::size_t operator()(const afterhours::EntityHandle& h) const noexcept {
        // Cheap hash; good enough for snapshot apply maps.
        const std::size_t a = static_cast<std::size_t>(h.slot);
        const std::size_t b = static_cast<std::size_t>(h.gen);
        return (a * 1315423911u) ^ (b + 0x9e3779b97f4a7c15ULL + (a << 6) +
                                   (a >> 2));
    }
};

struct EntityHandleEq {
    bool operator()(const afterhours::EntityHandle& a,
                    const afterhours::EntityHandle& b) const noexcept {
        return a.slot == b.slot && a.gen == b.gen;
    }
};

void remove_all_pooled_components_for(Entity& e) {
    // Prefer removing from Afterhours ComponentStore when available (pooled).
#if PHARMASEA_HAS_AFTERHOURS_COMPONENT_STORE
    // Mirror afterhours::EntityHelper::remove_pooled_components_for, but without
    // depending on vendor EntityHelper storage (PharmaSea owns entity arrays).
    for (afterhours::ComponentID cid = 0; cid < afterhours::max_num_components;
         ++cid) {
        if (!e.componentSet[cid]) continue;
        e.componentSet[cid] = false;
        afterhours::ComponentStore::get().remove_by_component_id(cid, e.id);
    }
#else
    // Fallback for older Afterhours builds (no pooled ComponentStore).
    // Best-effort: clear the presence bitset; any legacy per-entity component
    // storage (if still present) is owned by the entity and will be dropped when
    // we replace the entity list.
    e.componentSet.reset();
#endif
}

}  // namespace

WorldSnapshotV2 capture_from_entities(const Entities& entities) {
    WorldSnapshotV2 snap{};
    snap.entities.reserve(entities.size());

    for (const auto& sp : entities) {
        if (!sp) continue;
        const Entity& e = *sp;

        EntityRecordV2 rec{};
        rec.handle = EntityHelper::handle_for(e);
        rec.legacy_id = e.id;
        rec.entity_type = e.entity_type;
        rec.component_set = e.componentSet;
        rec.tags = e.tags;
        rec.cleanup = e.cleanup;
        rec.components_blob = snapshot_v2::encode_components_blob(e);
        snap.entities.push_back(rec);
    }

    return snap;
}

void apply_to_entities(Entities& entities, const WorldSnapshotV2& snap,
                       const ApplyOptionsV2 options) {
    // Remove ComponentStore-backed component data for the entities we are replacing.
    for (const auto& sp : entities) {
        if (!sp) continue;
        remove_all_pooled_components_for(*sp);
    }
    entities.clear();

    // Build entity records first so we can resolve component lists by handle.
    std::unordered_map<afterhours::EntityHandle, Entity*, EntityHandleHash,
                       EntityHandleEq>
        by_handle;
    by_handle.reserve(snap.entities.size());

    entities.reserve(snap.entities.size());
    for (const EntityRecordV2& rec : snap.entities) {
        auto sp = std::make_shared<Entity>();

        if (options.preserve_legacy_entity_ids) {
            // Preserve legacy EntityID-based relationships.
            // WARNING: unsafe when two worlds live in one process because
            // ComponentStore is global and keyed by EntityID.
            sp->id = rec.legacy_id;
        }
        sp->entity_type = rec.entity_type;
        sp->tags = rec.tags;
        sp->cleanup = rec.cleanup;

        // Ensure componentSet starts empty; we will rebuild via addComponent.
        sp->componentSet.reset();

        by_handle.emplace(rec.handle, sp.get());
        entities.push_back(std::move(sp));
    }

    // Apply component blobs (full component state).
    for (std::size_t i = 0; i < snap.entities.size(); ++i) {
        const auto& rec = snap.entities[i];
        if (i >= entities.size() || !entities[i]) continue;
        if (rec.components_blob.empty()) continue;
        snapshot_v2::decode_components_blob(*entities[i], rec.components_blob);
    }

    // Treat snapshot apply as an end-of-frame boundary for pooled components.
#if PHARMASEA_HAS_AFTERHOURS_COMPONENT_STORE
    afterhours::ComponentStore::get().flush_end_of_frame();
#endif

    // Invalidate any cached entity lookups (named entities, path cache, etc.).
    EntityHelper::invalidateCaches();
}

}  // namespace snapshot_v2

