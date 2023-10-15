#include "entity_helper.h"

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
        OptEntity opt_e = {};
        switch (name) {
            case NamedEntity::Sophie:
                opt_e = getFirstOfType(EntityType::Sophie);
                break;
        }
        if (!opt_e.has_value()) return {};
        named_entities_DO_NOT_USE.insert(
            std::make_pair(name, (Entity*) opt_e.value()));
    }
    return *(named_entities_DO_NOT_USE[name]);
}

Entity& EntityHelper::getNamedEntity(const NamedEntity& name) {
    log_trace("Trying to fetch {}", magic_enum::enum_name<NamedEntity>(name));
    OptEntity opt_e = getPossibleNamedEntity(name);
    if (!opt_e.has_value()) {
        log_error("Trying to fetch {} but didnt find it...",
                  magic_enum::enum_name<NamedEntity>(name));
    }
    return *(opt_e.value());
}

Entities& EntityHelper::get_entities() {
    if (is_server()) {
        return server_entities_DO_NOT_USE;
    }
    // Right now we only have server/client thread, but in the future if we
    // have more then we should check these

    // auto client_thread_id =
    // GLOBALS.get_or_default("client_thread_id", std::thread::id());
    return client_entities_DO_NOT_USE;
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
    get_entities().push_back(e);
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

    auto& entities = get_entities();

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
    Entities& entities = get_entities();

    auto newend = std::remove_if(
        entities.begin(), entities.end(),
        [](const auto& entity) { return !entity || entity->cleanup; });

    entities.erase(newend, entities.end());
}

void EntityHelper::delete_all_entities_NO_REALLY_I_MEAN_ALL() {
    Entities& entities = get_entities();
    // just clear the whole thing
    entities.clear();
}

void EntityHelper::delete_all_entities(bool include_permanent) {
    if (include_permanent) {
        delete_all_entities_NO_REALLY_I_MEAN_ALL();
        return;
    }

    // Only delete non perms
    Entities& entities = get_entities();

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

OptEntity EntityHelper::getFirstMatching(
    std::function<bool(RefEntity)> filter  //
) {
    for (const auto& e_ptr : get_entities()) {
        if (!e_ptr) continue;
        Entity& s = *e_ptr;
        if (!filter(s)) continue;
        return s;
    }
    return {};
}

OptEntity EntityHelper::getClosestMatchingFurniture(
    const Transform& transform, float range,
    std::function<bool(RefEntity)>&& filter) {
    // TODO :BE: should this really be using this?
    return EntityHelper::getMatchingEntityInFront(
        transform.as2(), range, transform.face_direction(), filter);
}

OptEntity EntityHelper::getEntityForID(EntityID id) {
    if (id == -1) return {};

    for (const auto& e : get_entities()) {
        if (!e) continue;
        if (e->id == id) return *e;
    }
    return {};
}

OptEntity EntityHelper::getClosestOfType(const Entity& entity,
                                         const EntityType& type, float range) {
    const Transform& transform = entity.get<Transform>();
    return EntityHelper::getClosestMatchingEntity(
        transform.as2(), range,
        [type](const RefEntity entity) { return check_type(entity, type); });
}

// TODO :BE: change other debugname filter guys to this
std::vector<RefEntity> EntityHelper::getAllWithType(const EntityType& type) {
    std::vector<RefEntity> matching;
    for (const std::shared_ptr<Entity>& e : get_entities()) {
        if (!e) continue;
        if (check_type(*e, type)) matching.push_back(*e);
    }
    return matching;
}

bool EntityHelper::doesAnyExistWithType(const EntityType& type) {
    for (std::shared_ptr<Entity> e : get_entities()) {
        if (!e) continue;
        if (check_type(*e, type)) return true;
    }
    return false;
}

bool EntityHelper::doesAnyExistMatchingFilter(
    const std::function<bool(RefEntity)>& filter) {
    for (const std::shared_ptr<Entity>& e : get_entities()) {
        if (!e) continue;
        if (filter(*e)) return true;
    }
    return false;
}

std::vector<RefEntity> EntityHelper::getFilteredEntitiesInRange(
    vec2 pos, float range, std::function<bool(RefEntity)> filter) {
    std::vector<RefEntity> matching;
    for (auto& e : get_entities()) {
        if (!e) continue;
        if (!filter(*e)) continue;
        if (vec::distance(pos, e->get<Transform>().as2()) < range) {
            matching.push_back(*e);
        }
    }
    return matching;
}

std::vector<RefEntity> EntityHelper::getEntitiesInRange(vec2 pos, float range) {
    return getFilteredEntitiesInRange(pos, range, [](auto&&) { return true; });
}

OptEntity EntityHelper::getMatchingEntityInFront(
    vec2 pos,                                 //
    float range,                              //
    Transform::FrontFaceDirection direction,  //
    std::function<bool(RefEntity)> filter     //
) {
    TRACY_ZONE_SCOPED;
    VALIDATE(range > 0,
             fmt::format("range has to be positive but was {}", range));

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
                          current_entity->template get<DebugName>().name());
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

OptEntity EntityHelper::getClosestMatchingEntity(
    vec2 pos, float range, std::function<bool(RefEntity)> filter) {
    float best_distance = range;
    OptEntity best_so_far = {};
    for (auto& e : get_entities()) {
        if (!e) continue;
        if (!filter(*e)) continue;
        float d = vec::distance(pos, e->get<Transform>().as2());
        if (d > range) continue;
        if (d < best_distance) {
            best_so_far = *e;
            best_distance = d;
        }
    }
    return best_so_far;
}

bool EntityHelper::hasOverlappingSolidEntitiesInRange(vec2 range_min,
                                                      vec2 range_max) {
    OptEntity e =
        EntityHelper::getOverlappingSolidEntityInRange(range_min, range_max);
    return e.valid();
}

RefEntities EntityHelper::getAllInRange(vec2 range_min, vec2 range_max) {
    RefEntities es;
    for (const std::shared_ptr<Entity>& e : get_entities()) {
        if (!e->has<IsSolid>()) continue;
        const auto pos = e->get<Transform>().as2();
        if (pos.x > range_max.x || pos.x < range_min.x) continue;
        if (pos.y > range_max.y || pos.y < range_min.y) continue;
        es.push_back(*e);
    }
    return es;
}

OptEntity EntityHelper::getOverlappingSolidEntityInRange(vec2 range_min,
                                                         vec2 range_max) {
    for (const std::shared_ptr<Entity>& e : get_entities()) {
        if (!e->has<IsSolid>()) continue;
        const auto pos = e->get<Transform>().as2();
        if (pos.x > range_max.x || pos.x < range_min.x) continue;
        if (pos.y > range_max.y || pos.y < range_min.y) continue;

        for (const std::shared_ptr<Entity>& e2 : get_entities()) {
            if (e2->id == e->id) continue;
            if (!e2->has<IsSolid>()) continue;
            const auto pos2 = e2->get<Transform>().as2();
            if (pos2.x > range_max.x || pos2.x < range_min.x) continue;
            if (pos2.y > range_max.y || pos2.y < range_min.y) continue;

            if (pos.x == pos2.x && pos.y == pos2.y) {
                return *e;
            }
        }
    }
    return {};
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
