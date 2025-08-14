#include "entity_helper.h"

#include "components/is_floor_marker.h"
#include "components/is_trigger_area.h"
#include "entity_query.h"
#include "system/input_process_manager.h"
#include <unordered_map>

// Maintain an O(1) id -> shared ownership handle via weak_ptr so the registry
// does not extend lifetimes on its own. Callers lock() to get the canonical
// shared_ptr from the owning vector.
using EntityRegistry = std::unordered_map<int, std::weak_ptr<Entity>>;

static EntityRegistry client_id_registry;
static EntityRegistry server_id_registry;

static inline EntityRegistry& get_registry_for_mod() {
    if (is_server()) return server_id_registry;
    return client_id_registry;
}

Entities client_entities_DO_NOT_USE;
Entities server_entities_DO_NOT_USE;
NamedEntities named_entities_DO_NOT_USE;

std::set<int> permanant_ids;
std::map<vec2, bool> cache_is_walkable;

///////////////////////////////////
///

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

OptEntity EntityHelper::getPossibleNamedEntity(const NamedEntity& name) {
    if (!is_server()) {
        // TODO this doesnt yet work on client side
        log_error(
            "This will not work on the client side yet (trying to fetch {})",
            magic_enum::enum_name<NamedEntity>(name));
    }
    bool has_in_cache = named_entities_DO_NOT_USE.contains(name);
    if (!has_in_cache) {
        Entity* e_ptr = nullptr;
        switch (name) {
            case NamedEntity::Sophie:
                OptEntity opt_e =
                    EntityQuery().whereType(EntityType::Sophie).gen_first();
                e_ptr = opt_e.has_value() ? opt_e.value() : nullptr;
                break;
        }
        if (!e_ptr) return {};
        // Store the canonical shared_ptr for safer hand-offs
        if (auto sp = EntityHelper::getEntityAsSharedPtr(*e_ptr)) {
            named_entities_DO_NOT_USE.insert(std::make_pair(name, sp));
        } else {
            return {};
        }
    }
    return *(named_entities_DO_NOT_USE[name]);
}

Entity& EntityHelper::getNamedEntity(const NamedEntity& name) {
    log_trace("Trying to fetch {}", magic_enum::enum_name<NamedEntity>(name));
    OptEntity opt_e = getPossibleNamedEntity(name);
    Entity* e_ptr = opt_e.has_value() ? opt_e.value() : nullptr;
    if (!e_ptr) {
        log_error("Trying to fetch {} but didnt find it...",
                  magic_enum::enum_name<NamedEntity>(name));
    }
    return *(e_ptr);
}

Entities& EntityHelper::get_entities_for_mod() {
    if (is_server()) {
        return server_entities_DO_NOT_USE;
    }
    // Right now we only have server/client thread, but in the future if we
    // have more then we should check these

    // auto client_thread_id =
    // GLOBALS.get_or_default("client_thread_id", std::thread::id());
    return client_entities_DO_NOT_USE;
}

const Entities& EntityHelper::get_entities() { return get_entities_for_mod(); }

RefEntities EntityHelper::get_ref_entities() {
    RefEntities matching;
    for (const auto& e : EntityHelper::get_entities()) {
        if (!e) continue;
        matching.push_back(*e);
    }
    return matching;
}

void EntityHelper::rebuild_id_registry() {
    auto& reg = get_registry_for_mod();
    reg.clear();
    for (const auto& e : get_entities_for_mod()) {
        if (!e) continue;
        reg[e->id] = e;
    }
}

struct CreationOptions {
    bool is_permanent;
};

Entity& EntityHelper::createEntity() {
    return createEntityWithOptions({.is_permanent = false});
}

Entity& EntityHelper::createPermanentEntity() {
    return createEntityWithOptions({.is_permanent = true});
}

Entity& EntityHelper::createEntityWithOptions(const CreationOptions& options) {
    std::shared_ptr<Entity> e(new Entity());
    get_entities_for_mod().push_back(e);
    // Update registry
    get_registry_for_mod()[e->id] = e;
    // log_info("created a new entity {}", e->id);

    invalidatePathCache();

    if (options.is_permanent) {
        permanant_ids.insert(e->id);
    }

    return *e;

    // if (!e->add_to_navmesh()) {
    // return;
    // }
    // auto nav = GLOBALS.get_ptr<NavMesh>("navmesh");
    // Note: addShape merges shapes next to each other
    //      this reduces the amount of loops overall

    // nav->addShape(getPolyForEntity(e));
    // nav->addEntity(e->id, getPolyForEntity(e));
    // cache_is_walkable.clear();
}

void EntityHelper::markIDForCleanup(int e_id) {
    // O(1) mark via registry
    auto& reg = get_registry_for_mod();
    auto it = reg.find(e_id);
    if (it != reg.end()) {
        if (auto sp = it->second.lock()) {
            sp->cleanup = true;
            return;
        }
    }
    // Fallback scan if registry entry missing/expired
    for (const auto& e : get_entities()) {
        if (!e) continue;
        if (e->id == e_id) {
            e->cleanup = true;
            break;
        }
    }
}

void EntityHelper::removeEntity(int e_id) {
    // if (e->add_to_navmesh()) {
    // auto nav = GLOBALS.get_ptr<NavMesh>("navmesh");
    // nav->removeEntity(e->id);
    // cache_is_walkable.clear();
    // }

    auto& entities = get_entities_for_mod();
    auto& reg = get_registry_for_mod();

    auto newend = std::remove_if(
        entities.begin(), entities.end(),
        [e_id](const auto& entity) { return !entity || entity->id == e_id; });

    entities.erase(newend, entities.end());

    // Remove from registry
    reg.erase(e_id);
}

//  Polygon getPolyForEntity(std::shared_ptr<Entity> e) {
// vec2 pos = vec::to2(e->snap_position());
// Rectangle rect = {
// pos.x,
// pos.y,
// TILESIZE,
// TILESIZE,
// };
// return Polygon(rect);
// }

void EntityHelper::cleanup() {
    // Cleanup entities marked cleanup
    Entities& entities = get_entities_for_mod();
    auto& reg = get_registry_for_mod();

    // Collect ids to remove from registry first
    std::vector<int> ids_to_erase;
    ids_to_erase.reserve(entities.size());
    for (const auto& entity : entities) {
        if (!entity) continue;
        if (entity->cleanup) ids_to_erase.push_back(entity->id);
    }
    for (int id : ids_to_erase) reg.erase(id);

    auto newend = std::remove_if(
        entities.begin(), entities.end(),
        [](const auto& entity) { return !entity || entity->cleanup; });

    entities.erase(newend, entities.end());
}

void EntityHelper::delete_all_entities_NO_REALLY_I_MEAN_ALL() {
    Entities& entities = get_entities_for_mod();
    // just clear the whole thing
    entities.clear();
    // and clear registry
    get_registry_for_mod().clear();
}

void EntityHelper::delete_all_entities(bool include_permanent) {
    if (include_permanent) {
        delete_all_entities_NO_REALLY_I_MEAN_ALL();
        return;
    }

    // Only delete non perms
    Entities& entities = get_entities_for_mod();
    auto& reg = get_registry_for_mod();

    // Remove registry entries for non-permanent entities
    for (const auto& entity : entities) {
        if (!entity) continue;
        if (!permanant_ids.contains(entity->id)) reg.erase(entity->id);
    }

    auto newend = std::remove_if(
        entities.begin(), entities.end(),
        [](const auto& entity) { return !permanant_ids.contains(entity->id); });

    entities.erase(newend, entities.end());
}

enum ForEachFlow {
    NormalFlow = 0,
    Continue = 1,
    Break = 2,
};

void EntityHelper::forEachEntity(std::function<ForEachFlow(Entity&)> cb) {
    TRACY_ZONE_SCOPED;
    for (const auto& e : get_entities()) {
        if (!e) continue;
        auto fef = cb(*e);
        if (fef == 1) continue;
        if (fef == 2) break;
    }
}

OptEntity EntityHelper::getClosestMatchingFurniture(
    const Transform& transform, float range,
    const std::function<bool(const Entity&)>& filter) {
    // TODO :BE: should this really be using this?
    return EntityHelper::getMatchingEntityInFront(
        transform.as2(), range, transform.face_direction(), filter);
}

OptEntity EntityHelper::getEntityForID(EntityID id) {
    if (id == -1) return {};
    auto& reg = get_registry_for_mod();
    auto it = reg.find(id);
    if (it == reg.end()) return {};
    if (auto sp = it->second.lock()) return *sp;
    // Entry exists but expired, drop it and report not found
    reg.erase(it);
    return {};
}

OptEntity EntityHelper::getClosestOfType(const Entity& entity,
                                         const EntityType& type, float range) {
    const Transform& transform = entity.get<Transform>();
    return EntityQuery()
        .whereType(type)
        .whereInRange(transform.as2(), range)
        .orderByDist(transform.as2())
        .gen_first();
}

OptEntity EntityHelper::getMatchingFloorMarker(IsFloorMarker::Type type) {
    return EntityQuery()
        .whereHasComponentAndLambda<IsFloorMarker>(
            [type](const IsFloorMarker& fm) { return fm.type == type; })
        .gen_first();
}

OptEntity EntityHelper::getMatchingTriggerArea(IsTriggerArea::Type type) {
    return EntityQuery()
        .whereHasComponentAndLambda<IsTriggerArea>(
            [type](const IsTriggerArea& ta) { return ta.type == type; })
        .gen_first();
}

// TODO: make this more explicit that we are ignoring store entities
// (this was already the default but new callers should know)
bool EntityHelper::doesAnyExistWithType(const EntityType& type) {
    return EntityQuery().whereType(type).has_values();
}

std::shared_ptr<Entity> EntityHelper::getEntityAsSharedPtr(
    const Entity& entity) {
    auto& reg = get_registry_for_mod();
    auto it = reg.find(entity.id);
    if (it == reg.end()) return {};
    return it->second.lock();
}

std::shared_ptr<Entity> EntityHelper::getEntityAsSharedPtr(OptEntity entity) {
    if (!entity) return {};
    return getEntityAsSharedPtr(entity.asE());
}

OptEntity EntityHelper::getMatchingEntityInFront(
    vec2 pos,                                         //
    float range,                                      //
    Transform::FrontFaceDirection direction,          //
    const std::function<bool(const Entity&)>& filter  //
) {
    TRACY_ZONE_SCOPED;
    VALIDATE(range > 0,
             fmt::format("range has to be positive but was {}", range));
    /**
        auto& eq = EntityQuery()
                   .whereLambda(filter)
                   .whereInFront(tile)
                   .whereSnappedPositionMatches(vec::snap(tile));
        if (eq.has_values()) {
            return eq.gen_first();
        }
     */

    int cur_step = 0;
    int irange = static_cast<int>(range);
    while (cur_step <= irange) {
        auto tile = Transform::tile_infront_given_pos(pos, cur_step, direction);

        for (std::shared_ptr<Entity> current_entity : get_entities()) {
            if (!current_entity) continue;
            if (!filter(*current_entity)) continue;

            // all entitites should have transforms but just in case
            if (current_entity->template is_missing<Transform>()) {
                log_warn("component {} is missing transform",
                         current_entity->id);
                log_error("component {} is missing name",
                          current_entity->get<Type>().name());
                continue;
            }

            const Transform& transform =
                current_entity->template get<Transform>();

            float cur_dist = vec::distance(transform.as2(), tile);
            // outside reach
            if (abs(cur_dist) > 1) continue;
            // this is behind us
            if (cur_dist < 0) continue;

            // TODO :BE: add a snap_as2() function to transform
            if (vec::to2(transform.snap_position()) == vec::snap(tile)) {
                return *current_entity;
            }
        }
        cur_step++;
    }
    return {};
}

RefEntities EntityHelper::getAllInRangeFiltered(
    vec2 range_min, vec2 range_max,
    const std::function<bool(const Entity&)>& filter) {
    return EntityQuery()
        .whereInside(range_min, range_max)
        // idk its called all
        .include_store_entities()
        .whereLambda(filter)
        .gen();
}

RefEntities EntityHelper::getAllInRange(vec2 range_min, vec2 range_max) {
    return EntityQuery()
        .whereInside(range_min, range_max)
        // idk its called all
        .include_store_entities()
        .gen();
}

OptEntity EntityHelper::getOverlappingEntityIfExists(
    const Entity& entity, float range,
    const std::function<bool(const Entity&)>& filter,
    bool include_store_entities) {
    const vec2 position = entity.get<Transform>().as2();
    return EntityQuery()                                 //
        .whereNotID(entity.id)                           //
        .whereLambdaExistsAndTrue(filter)                //
        .whereHasComponent<IsSolid>()                    //
        .whereInRange(position, range)                   //
        .wherePositionMatches(entity)                    //
        .include_store_entities(include_store_entities)  //
        .gen_first();
}

// TODO :INFRA: i think this is slower because we are doing "outside mesh"
// as outside we should probably have just make some tiles for inside the
// map
// ('.' on map for example) and use those to mark where people can walk and
// where they cant
// static bool isWalkable_impl(const vec2& pos) {
// auto nav = GLOBALS.get_ptr<NavMesh>("navmesh");
// if (!nav) {
// return true;
// }
//
// for (auto kv : nav->entityShapes) {
// auto s = kv.second;
// if (s.inside(pos)) return false;
// }
// return true;
// }
//
void EntityHelper::invalidateCaches() {
    named_entities_DO_NOT_USE.clear();
    invalidatePathCache();
}

// TODO :PBUG: need to invalidate any current valid paths
void EntityHelper::invalidatePathCacheLocation(vec2 pos) {
    if (!is_server()) {
        // Turning off the warning because of input_process_manager _proc_single
        // log_warn("client code is trying to invalide path cache");
        return;
    }
    cache_is_walkable.erase(pos);
}

void EntityHelper::invalidatePathCache() {
    if (!is_server()) {
        // Turning off the warning because of input_process_manager _proc_single
        // log_warn("client code is trying to invalide path cache");
        return;
    }
    cache_is_walkable.clear();
}

bool EntityHelper::isWalkable(vec2 pos) {
    TRACY_ZONE_SCOPED;
    if (!cache_is_walkable.contains(pos)) {
        bool walkable = isWalkableRawEntities(pos);
        cache_is_walkable[pos] = walkable;
    }
    return cache_is_walkable[pos];
}

// each target.get and path_find runs through all entities
// so this will just get slower and slower over time
bool EntityHelper::isWalkableRawEntities(const vec2& pos) {
    TRACY_ZONE_SCOPED;
    bool hit_impassible_entity = false;
    forEachEntity([&](Entity& entity) {
        // Ignore non colliable objects
        if (!system_manager::input_process_manager::is_collidable(entity))
            return ForEachFlow::Continue;
        // ignore things that are not at this location
        if (vec::distance(entity.template get<Transform>().as2(), pos) >
            TILESIZE / 2.f)
            return ForEachFlow::Continue;

        // is_collidable and inside this square
        hit_impassible_entity = true;
        return ForEachFlow::Break;
    });
    return !hit_impassible_entity;
}

#pragma clang diagnostic pop
