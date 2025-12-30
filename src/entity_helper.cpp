#include "entity_helper.h"

#include "components/is_floor_marker.h"
#include "components/is_trigger_area.h"
#include "entity_id.h"
#include "entity_query.h"
#include "entity_type.h"
#include "system/input_process_manager.h"

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
                    EQ().whereType(EntityType::Sophie).gen_first();
                e_ptr = opt_e.has_value() ? opt_e.value() : nullptr;
                break;
        }
        if (!e_ptr) return {};

        // IMPORTANT: never construct a new shared_ptr from a raw Entity* here.
        // Entities are already owned by the main entity list. Creating a new
        // control block would cause double-free when caches are cleared.
        std::shared_ptr<Entity> owned;
        for (const auto& sp : EntityHelper::get_entities_for_mod()) {
            if (sp && sp.get() == e_ptr) {
                owned = sp;
                break;
            }
        }
        if (!owned) {
            log_error(
                "Named entity cache could not find owning shared_ptr for {} (id {})",
                magic_enum::enum_name<NamedEntity>(name), e_ptr->id);
            return {};
        }
        named_entities_DO_NOT_USE.insert(std::make_pair(name, owned));
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
    auto e = std::make_shared<Entity>();
    get_entities_for_mod().push_back(e);

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
    auto& entities = get_entities();
    auto it = entities.begin();
    while (it != get_entities().end()) {
        if ((*it)->id == e_id) {
            (*it)->cleanup = true;
            break;
        }
        it++;
    }
}

void EntityHelper::removeEntity(int e_id) {
    // if (e->add_to_navmesh()) {
    // auto nav = GLOBALS.get_ptr<NavMesh>("navmesh");
    // nav->removeEntity(e->id);
    // cache_is_walkable.clear();
    // }

    auto& entities = get_entities_for_mod();

    auto newend = std::remove_if(
        entities.begin(), entities.end(),
        [e_id](const auto& entity) { return !entity || entity->id == e_id; });

    entities.erase(newend, entities.end());
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

    auto newend = std::remove_if(
        entities.begin(), entities.end(),
        [](const auto& entity) { return !entity || entity->cleanup; });

    entities.erase(newend, entities.end());
}

void EntityHelper::delete_all_entities_NO_REALLY_I_MEAN_ALL() {
    Entities& entities = get_entities_for_mod();
    // just clear the whole thing
    entities.clear();
}

void EntityHelper::delete_all_entities(bool include_permanent) {
    if (include_permanent) {
        delete_all_entities_NO_REALLY_I_MEAN_ALL();
        return;
    }

    // Only delete non perms
    Entities& entities = get_entities_for_mod();

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
    if (id == entity_id::INVALID) return {};

    for (const auto& e : get_entities()) {
        if (!e) continue;
        if (e->id == id) return *e;
    }
    return {};
}

Entity& EntityHelper::getEnforcedEntityForID(EntityID id) {
    OptEntity opt = getEntityForID(id);
    if (!opt) {
        log_error("EntityHelper::getEnforcedEntityForID failed: {}", id);
    }
    return opt.asE();
}

OptEntity EntityHelper::getClosestOfType(const Entity& entity,
                                         const EntityType& type, float range) {
    const Transform& transform = entity.get<Transform>();
    return EQ()
        .whereType(type)
        .whereInRange(transform.as2(), range)
        .orderByDist(transform.as2())
        .gen_first();
}

OptEntity EntityHelper::getMatchingFloorMarker(IsFloorMarker::Type type) {
    return EQ()
        .whereHasComponentAndLambda<IsFloorMarker>(
            [type](const IsFloorMarker& fm) { return fm.type == type; })
        .gen_first();
}

OptEntity EntityHelper::getMatchingTriggerArea(IsTriggerArea::Type type) {
    return EQ()
        .whereHasComponentAndLambda<IsTriggerArea>(
            [type](const IsTriggerArea& ta) { return ta.type == type; })
        .gen_first();
}

// TODO: make this more explicit that we are ignoring store entities
// (this was already the default but new callers should know)
bool EntityHelper::doesAnyExistWithType(const EntityType& type) {
    return EQ().whereType(type).has_values();
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
        auto& eq = EQ()
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
                          str(get_entity_type(*current_entity)));
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
    return EQ()
        .whereInside(range_min, range_max)
        // idk its called all
        .include_store_entities()
        .whereLambda(filter)
        .gen();
}

RefEntities EntityHelper::getAllInRange(vec2 range_min, vec2 range_max) {
    return EQ()
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
    return EQ()                                          //
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
