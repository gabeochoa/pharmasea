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

using EntityIdRemap =
    std::unordered_map<afterhours::EntityID, afterhours::EntityID>;

[[nodiscard]] afterhours::EntityID remap_id_or(
    const EntityIdRemap& remap, const afterhours::EntityID old_id,
    const afterhours::EntityID fallback) {
    auto it = remap.find(old_id);
    if (it == remap.end()) return fallback;
    return it->second;
}

void remap_entity_id_fields(Entities& entities, const EntityIdRemap& remap) {
    if (remap.empty()) return;

    const auto f = [&](afterhours::EntityID id) -> afterhours::EntityID {
        return remap_id_or(remap, id, id);
    };

    for (auto& sp : entities) {
        if (!sp) continue;
        Entity& e = *sp;

        if (e.has<CanHoldItem>()) {
            e.get<CanHoldItem>().remap_entity_ids(f);
        }
        if (e.has<CanHoldFurniture>()) {
            auto& chf = e.get<CanHoldFurniture>();
            if (chf.is_holding_furniture()) {
                const auto old = chf.furniture_id();
                const auto nw = (int)f(old);
                if (nw != old) chf.update(nw, chf.picked_up_at());
            }
        }
        if (e.has<CanHoldHandTruck>()) {
            auto& cht = e.get<CanHoldHandTruck>();
            if (cht.is_holding()) {
                const auto old = cht.hand_truck_id();
                const auto nw = (int)f(old);
                if (nw != old) cht.update(nw, cht.picked_up_at());
            }
        }
        if (e.has<HasWaitingQueue>()) {
            e.get<HasWaitingQueue>().remap_entity_ids(f);
        }
        if (e.has<HasRopeToItem>()) {
            e.get<HasRopeToItem>().remap_entity_ids(f);
        }
        if (e.has<HasLastInteractedCustomer>()) {
            auto& hl = e.get<HasLastInteractedCustomer>();
            if (hl.customer_id != -1) {
                hl.customer_id = (int)f(hl.customer_id);
            }
        }
        if (e.has<IsFloorMarker>()) {
            const auto& ids = e.get<IsFloorMarker>().marked_ids();
            if (!ids.empty()) {
                std::vector<int> out;
                out.reserve(ids.size());
                for (int id : ids) out.push_back((int)f(id));
                e.get<IsFloorMarker>().mark_all(std::move(out));
            }
        }
        if (e.has<IsSquirter>()) {
            auto& sq = e.get<IsSquirter>();
            if (sq.item_id() != -1) {
                const int old = sq.item_id();
                const int nw = (int)f(old);
                if (nw != old) sq.update(nw, sq.picked_up_at());
            }
            if (sq.drink_id() != -1) {
                const int old = sq.drink_id();
                const int nw = (int)f(old);
                if (nw != old) sq.set_drink_id(nw);
            }
        }

        // Components that use "parent_id" patterns.
        if (e.has<CanPathfind>()) {
            // Parent is expected to be this entity; remap defensively anyway.
            e.get<CanPathfind>().set_parent((int)f(e.id));
        }
        if (e.has<RespondsToDayNight>()) {
            e.get<RespondsToDayNight>().set_parent((int)f(e.id));
        }
        if (e.has<AddsIngredient>()) {
            e.get<AddsIngredient>().set_parent((int)f(e.id));
        }
    }
}

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
    // Reused scratch buffer to avoid per-entity allocations.
    std::vector<std::uint8_t> scratch;
    scratch.reserve(64 * 1024);

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

        snapshot_v2::encode_components_blob_into(e, scratch);
        rec.components_offset = static_cast<std::uint32_t>(snap.component_bytes.size());
        rec.components_size = static_cast<std::uint32_t>(scratch.size());
        if (rec.components_size > snapshot_v2::kMaxSnapshotEntityComponentBytes) {
            log_warn("snapshot_v2: entity {} component bytes too large: {}",
                     rec.legacy_id, rec.components_size);
        }
        snap.component_bytes.insert(snap.component_bytes.end(), scratch.begin(),
                                    scratch.end());
        snap.entities.push_back(rec);
    }

    return snap;
}

void apply_to_entities(Entities& entities, const WorldSnapshotV2& snap,
                       const ApplyOptionsV2 options) {
    if (options.clear_existing_components) {
        // Remove ComponentStore-backed component data for the entities we are replacing.
        for (const auto& sp : entities) {
            if (!sp) continue;
            remove_all_pooled_components_for(*sp);
        }
    }
    entities.clear();

    // Build entity records first so we can resolve component lists by handle.
    std::unordered_map<afterhours::EntityHandle, Entity*, EntityHandleHash,
                       EntityHandleEq>
        by_handle;
    by_handle.reserve(snap.entities.size());

    entities.reserve(snap.entities.size());
    EntityIdRemap id_remap;
    id_remap.reserve(snap.entities.size());

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
        // Record mapping for later fixups when we *don't* preserve IDs.
        if (!options.preserve_legacy_entity_ids) {
            id_remap.emplace(rec.legacy_id, sp->id);
        }
        entities.push_back(std::move(sp));
    }

    // Apply component blobs (full component state).
    for (std::size_t i = 0; i < snap.entities.size(); ++i) {
        const auto& rec = snap.entities[i];
        if (i >= entities.size() || !entities[i]) continue;
        if (rec.components_size == 0) continue;
        const std::size_t off = rec.components_offset;
        const std::size_t sz = rec.components_size;
        if (off + sz > snap.component_bytes.size()) {
            log_error("snapshot_v2: bad component slice off={} size={} arena={}",
                      off, sz, snap.component_bytes.size());
            continue;
        }
        snapshot_v2::decode_components_blob(*entities[i], snap.component_bytes.data() + off, sz);
    }

    if (!options.preserve_legacy_entity_ids) {
        remap_entity_id_fields(entities, id_remap);
    }

    // Treat snapshot apply as an end-of-frame boundary for pooled components.
#if PHARMASEA_HAS_AFTERHOURS_COMPONENT_STORE
    afterhours::ComponentStore::get().flush_end_of_frame();
#endif

    // Invalidate any cached entity lookups (named entities, path cache, etc.).
    EntityHelper::invalidateCaches();
}

}  // namespace snapshot_v2

