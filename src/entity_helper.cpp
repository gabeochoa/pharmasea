#include "entity_helper.h"

#include "components/is_floor_marker.h"
#include "components/is_trigger_area.h"
#include "entity_id.h"
#include "entity_query.h"
#include "entity_type.h"
#include "system/input_process_manager.h"

// Thread-specific EntityCollections
// Each thread manages its own collection independently
afterhours::EntityCollection client_collection;
afterhours::EntityCollection server_collection;

NamedEntities named_entities_DO_NOT_USE;
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
                "Named entity cache could not find owning shared_ptr for {} "
                "(id {})",
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

RefEntities EntityHelper::get_ref_entities() {
    RefEntities matching;
    for (const auto& e : EntityHelper::get_entities()) {
        if (!e) continue;
        matching.push_back(*e);
    }
    return matching;
}

Entity& EntityHelper::createEntity() {
    return createEntityWithOptions({.is_permanent = false});
}

Entity& EntityHelper::createPermanentEntity() {
    return createEntityWithOptions({.is_permanent = true});
}

Entity& EntityHelper::createEntityWithOptions(
    const EntityHelper::CreationOptions& options) {
    auto& collection = EntityHelper::get_current_collection();
    afterhours::EntityCollection::CreationOptions ah_options;
    ah_options.is_permanent = options.is_permanent;

    Entity& e = collection.createEntityWithOptions(ah_options);
    EntityHelper::invalidatePathCache();
    return e;

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

enum ForEachFlow {
    NormalFlow = 0,
    Continue = 1,
    Break = 2,
};

void EntityHelper::forEachEntity(
    const std::function<
        afterhours::EntityHelper::ForEachFlow(afterhours::Entity&)>& cb) {
    TRACY_ZONE_SCOPED;
    for (const auto& e : EntityHelper::get_entities()) {
        if (!e) continue;
        const auto fef = cb(*e);
        if (fef == afterhours::EntityHelper::ForEachFlow::Continue) continue;
        if (fef == afterhours::EntityHelper::ForEachFlow::Break) break;
    }
}

OptEntity EntityHelper::getClosestMatchingFurniture(
    const Transform& transform, float range,
    const std::function<bool(const Entity&)>& filter) {
    // TODO :BE: should this really be using this?
    return EntityHelper::getMatchingEntityInFront(
        transform.as2(), range, transform.face_direction(), filter);
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

        for (std::shared_ptr<Entity> current_entity :
             EntityHelper::get_entities()) {
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
    EntityHelper::invalidatePathCache();
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
    EntityHelper::forEachEntity([&](afterhours::Entity& entity) {
        // Ignore non colliable objects
        if (!system_manager::input_process_manager::is_collidable(entity))
            return EntityHelper::ForEachFlow::Continue;
        // ignore things that are not at this location
        if (vec::distance(entity.template get<Transform>().as2(), pos) >
            TILESIZE / 2.f)
            return EntityHelper::ForEachFlow::Continue;

        // is_collidable and inside this square
        hit_impassible_entity = true;
        return EntityHelper::ForEachFlow::Break;
    });
    return !hit_impassible_entity;
}

#pragma clang diagnostic pop
