
#pragma once

#include <thread>

#include "assert.h"
#include "components/can_hold_item.h"
#include "components/debug_name.h"
#include "components/transform.h"
#include "external_include.h"
//
#include "components/can_be_ghost_player.h"
#include "components/can_be_held.h"
#include "components/is_solid.h"
#include "engine/globals_register.h"
#include "engine/is_server.h"
#include "globals.h"

//
#include "engine/statemanager.h"
#include "entity.h"
#include "entity_makers.h"
#include "job.h"
#include "strings.h"
// TODO :BE: eventually move to input manager but for now has to be in here
// to prevent circular includes

// return true if the item has collision and is currently collidable
[[nodiscard]] inline bool is_collidable(
    std::shared_ptr<Entity> entity, std::shared_ptr<Entity> other = nullptr) {
    if (!entity) return false;

    // by default we disable collisions when you are holding something
    // since its generally inside your bounding box
    if (entity->has<CanBeHeld>() && entity->get<CanBeHeld>().is_held()) {
        return false;
    }

    if (
        // checking for person update
        other != nullptr &&
        // Entity is item and held by player
        entity->has<IsItem>() &&
        entity->get<IsItem>().is_held_by(EntityType::Player) &&
        // Entity is rope
        check_type(*entity, EntityType::SodaSpout) &&
        // we are a player that is holding rope
        other->has<CanHoldItem>() &&
        other->get<CanHoldItem>().is_holding_item() &&
        check_type(*other->get<CanHoldItem>().item(), EntityType::SodaSpout)) {
        return false;
    }

    if (entity->has<IsSolid>()) {
        return true;
    }

    // TODO :BE: rename this since it no longer makes sense
    // if you are a ghost player
    // then you are collidable
    if (entity->has<CanBeGhostPlayer>()) {
        return true;
    }
    return false;
}

typedef std::vector<std::shared_ptr<Entity>> Entities;
extern Entities client_entities_DO_NOT_USE;
extern Entities server_entities_DO_NOT_USE;

extern std::set<int> permanant_ids;
extern std::map<vec2, bool> cache_is_walkable;

struct EntityHelper {
    struct CreationOptions {
        bool is_permanent;
    };

    static Entities& get_entities();

    static Entity& createEntity();
    static Entity& createPermanentEntity();
    static Entity& createEntityWithOptions(const CreationOptions& options);

    // TODO :BE: eventually return the entity id or something
    template<typename... TArgs>
    static std::shared_ptr<Entity> createItem(TArgs... args) {
        Entity& e = createEntity();
        items::make_item_type(e, std::forward<TArgs>(args)...);
        // log_info("created a new item {} {} ", e.id, e.get<DebugName>());
        return get_entities().back();
    }

    template<typename... TArgs>
    static std::shared_ptr<Entity> createPermanentItem(TArgs... args) {
        Entity& e = createPermanentEntity();
        items::make_item_type(e, std::forward<TArgs>(args)...);
        // log_info("created a new item {} {} ", e.id, e.get<DebugName>());
        return get_entities().back();
    }

    static void markIDForCleanup(int e_id);
    static void removeEntity(int e_id);
    static void removeEntity(std::shared_ptr<Entity> e);
    static void cleanup();
    static void delete_all_entities_NO_REALLY_I_MEAN_ALL();
    static void delete_all_entities(bool include_permanent = false);

    enum ForEachFlow {
        NormalFlow = 0,
        Continue = 1,
        Break = 2,
    };

    static void forEachEntity(
        std::function<ForEachFlow(std::shared_ptr<Entity>&)> cb);

    static std::vector<std::shared_ptr<Entity>> getFilteredEntitiesInRange(
        vec2 pos, float range,
        std::function<bool(std::shared_ptr<Entity>)> filter);

    static std::vector<std::shared_ptr<Entity>> getEntitiesInRange(vec2 pos,
                                                                   float range);

    static OptEntity getFirstMatching(std::function<bool(RefEntity)> filter);
    static std::vector<std::shared_ptr<Entity>> getEntitiesInPosition(
        vec2 pos) {
        return getEntitiesInRange(pos, TILESIZE);
    }

    static std::shared_ptr<Entity> getClosestMatchingFurniture(
        const Transform& transform, float range,
        std::function<bool(std::shared_ptr<Furniture>)> filter);

    static std::shared_ptr<Entity> getEntityPtrForID(EntityID id);
    static OptEntity getEntityForID(EntityID id);
    static std::shared_ptr<Entity> getClosestOfType(
        const std::shared_ptr<Entity>& entity, const EntityType& type,
        float range = 100.f);

    static std::shared_ptr<Entity> getClosestOfType(const Entity& entity,
                                                    const EntityType& type,
                                                    float range = 100.f);

    // TODO :BE: change other debugname filter guys to this
    static std::vector<std::shared_ptr<Entity>> getAllWithType(
        const EntityType& type);

    static bool doesAnyExistWithType(const EntityType& type);

    static std::shared_ptr<Entity> getMatchingEntityInFront(
        vec2 pos,                                            //
        float range,                                         //
        Transform::FrontFaceDirection direction,             //
        std::function<bool(std::shared_ptr<Entity>)> filter  //
    );

    static std::shared_ptr<Entity> getClosestMatchingEntity(
        vec2 pos, float range,
        std::function<bool(std::shared_ptr<Entity>)> filter);

    template<typename T>
    static std::shared_ptr<Entity> getClosestWithComponent(
        const std::shared_ptr<Entity>& entity, float range) {
        const Transform& transform = entity->get<Transform>();
        return EntityHelper::getClosestMatchingEntity(
            transform.as2(), range, [](const std::shared_ptr<Entity> entity) {
                return entity->has<T>();
            });
    }

    template<typename T>
    static std::vector<std::shared_ptr<Entity>> getAllWithComponent() {
        std::vector<std::shared_ptr<Entity>> matching;
        for (auto& e : get_entities()) {
            if (!e) continue;
            if (e->has<T>()) matching.push_back(e);
        }
        return matching;
    }

    template<typename T>
    static std::shared_ptr<Entity> getFirstWithComponent() {
        for (const auto& e : get_entities()) {
            if (!e) continue;
            if (e->has<T>()) return e;
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
    static inline void invalidatePathCacheLocation(vec2 pos) {
        cache_is_walkable.erase(pos);
    }

    static inline void invalidatePathCache() { cache_is_walkable.clear(); }

    static inline bool isWalkable(vec2 pos) {
        TRACY_ZONE_SCOPED;
        if (!cache_is_walkable.contains(pos)) {
            bool walkable = isWalkableRawEntities(pos);
            cache_is_walkable[pos] = walkable;
        }
        return cache_is_walkable[pos];
    }

    // each target get and path find runs through all entities
    // so this will just get slower and slower over time
    static inline bool isWalkableRawEntities(const vec2& pos) {
        TRACY_ZONE_SCOPED;
        bool hit_impassible_entity = false;
        forEachEntity([&](auto entity) {
            if (!is_collidable(entity)) return ForEachFlow::Continue;
            if (vec::distance(entity->template get<Transform>().as2(), pos) <
                TILESIZE / 2.f) {
                hit_impassible_entity = true;
                return ForEachFlow::Break;
            }
            return ForEachFlow::Continue;
        });
        return !hit_impassible_entity;
    }
};
