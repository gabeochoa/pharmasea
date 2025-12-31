#include "world_snapshot_v2_runtime.h"

#include "ah.h"
#include "engine/log.h"

#include <unordered_map>

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
    // Mirror afterhours::EntityHelper::remove_pooled_components_for, but without
    // depending on vendor EntityHelper storage (PharmaSea owns entity arrays).
    for (afterhours::ComponentID cid = 0; cid < afterhours::max_num_components;
         ++cid) {
        if (!e.componentSet[cid]) continue;
        e.componentSet[cid] = false;
        afterhours::ComponentStore::get().remove_by_component_id(cid, e.id);
    }
}

[[nodiscard]] TransformV2 project_transform(const Transform& t) {
    TransformV2 out{};
    out.visual_offset = t.viz_offset();
    out.raw_position = t.raw();
    out.position = t.pos();
    out.facing = t.facing;
    out.size = t.size();
    return out;
}

void apply_transform(Entity& e, const TransformV2& v) {
    // For initial wiring we enforce existence + write fields via public API.
    Transform& t = e.addComponentIfMissing<Transform>();
    t.init(v.raw_position, v.size);
    t.update_visual_offset(v.visual_offset);
    t.update_face_direction(v.facing);
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
        snap.entities.push_back(rec);

        if (e.has<Transform>()) {
            snap.components.transform.entries.push_back(
                {.entity = rec.handle, .value = project_transform(e.get<Transform>())});
        }
    }

    return snap;
}

void apply_to_entities(Entities& entities, const WorldSnapshotV2& snap) {
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

        // Override id to preserve legacy EntityID-based relationships.
        sp->id = rec.legacy_id;
        sp->entity_type = rec.entity_type;
        sp->tags = rec.tags;
        sp->cleanup = rec.cleanup;

        // Ensure componentSet starts empty; we will rebuild via addComponent.
        sp->componentSet.reset();

        by_handle.emplace(rec.handle, sp.get());
        entities.push_back(std::move(sp));
    }

    // Apply Transform list.
    for (const auto& entry : snap.components.transform.entries) {
        auto it = by_handle.find(entry.entity);
        if (it == by_handle.end() || !it->second) continue;
        apply_transform(*it->second, entry.value);
    }

    // Treat snapshot apply as an end-of-frame boundary for pooled components.
    afterhours::ComponentStore::get().flush_end_of_frame();

    // Invalidate any cached entity lookups (named entities, path cache, etc.).
    EntityHelper::invalidateCaches();
}

}  // namespace snapshot_v2

