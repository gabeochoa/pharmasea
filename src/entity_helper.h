
#pragma once

#include <thread>

#include "assert.h"
#include "components/can_hold_item.h"
#include "components/debug_name.h"
#include "components/is_floor_marker.h"
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

enum struct NamedEntity {
    Sophie,
};

using Entities = std::vector<std::shared_ptr<Entity>>;
using RefEntities = std::vector<RefEntity>;
using NamedEntities = std::map<NamedEntity, std::shared_ptr<Entity>>;

extern Entities client_entities_DO_NOT_USE;
extern NamedEntities named_entities_DO_NOT_USE;

extern std::set<int> permanant_ids;
extern std::map<vec2, bool> cache_is_walkable;

struct EntityHelper {
    struct CreationOptions {
        bool is_permanent;
    };

    static Entities& get_entities();
    static RefEntities get_ref_entities();

    static Entity& createEntity();
    static Entity& createPermanentEntity();
    static Entity& createEntityWithOptions(const CreationOptions& options);

    static Entity& getNamedEntity(const NamedEntity& name);
    static OptEntity getPossibleNamedEntity(const NamedEntity& name);

    // TODO :BE: maybe return the entity id or something
    template<typename... TArgs>
    static RefEntity createItem(TArgs... args) {
        items::make_item_type(createEntity(), std::forward<TArgs>(args)...);
        // log_info("created a new item {} {} ", e.id, e.get<DebugName>());
        return *(get_entities().back());
    }

    template<typename... TArgs>
    static RefEntity createPermanentItem(TArgs... args) {
        items::make_item_type(createPermanentEntity(),
                              std::forward<TArgs>(args)...);
        // log_info("created a new item {} {} ", e.id, e.get<DebugName>());
        return *(get_entities().back());
    }

    static void markIDForCleanup(int e_id);
    static void removeEntity(int e_id);
    static void cleanup();
    static void delete_all_entities_NO_REALLY_I_MEAN_ALL();
    static void delete_all_entities(bool include_permanent = false);

    enum ForEachFlow {
        NormalFlow = 0,
        Continue = 1,
        Break = 2,
    };

    static void forEachEntity(std::function<ForEachFlow(Entity&)> cb);

    static std::vector<RefEntity> getFilteredEntitiesInRange(
        vec2 pos, float range,
        const std::function<bool(const Entity&)>& filter);

    static std::vector<RefEntity> getEntitiesInRange(vec2 pos, float range);

    static std::vector<RefEntity> getEntitiesInPosition(vec2 pos) {
        return getEntitiesInRange(pos, TILESIZE);
    }

    // TODO exists as a conversion for things that need shared_ptr right now
    static std::shared_ptr<Entity> getEntityAsSharedPtr(OptEntity entity) {
        if (!entity) return {};
        for (std::shared_ptr<Entity> current_entity : get_entities()) {
            if (entity->id == current_entity->id) return current_entity;
        }
        return {};
    }

    static OptEntity getClosestMatchingFurniture(
        const Transform& transform, float range,
        const std::function<bool(const Entity&)>& filter);

    static OptEntity getEntityForID(EntityID id);

    static OptEntity getClosestOfType(const Entity& entity,
                                      const EntityType& type,
                                      float range = 100.f);

    // TODO :BE: change other debugname filter guys to this

    static bool doesAnyExistWithType(const EntityType& type);

    static OptEntity getMatchingEntityInFront(
        vec2 pos,                                         //
        float range,                                      //
        Transform::FrontFaceDirection direction,          //
        const std::function<bool(const Entity&)>& filter  //
    );

    static OptEntity getClosestMatchingEntity(
        vec2 pos, float range,
        const std::function<bool(const Entity&)>& filter);

    template<typename T>
    static OptEntity getClosestWithComponent(const Entity& entity,
                                             float range) {
        const Transform& transform = entity.get<Transform>();
        return EntityHelper::getClosestMatchingEntity(
            transform.as2(), range,
            [](const Entity& entity) { return entity.has<T>(); });
    }

    static RefEntities getAllInRange(vec2 range_min, vec2 range_max);
    static RefEntities getAllInRangeFiltered(
        vec2 range_min, vec2 range_max,
        const std::function<bool(const Entity&)>& filter);

    static OptEntity getOverlappingSolidEntityInRange(
        vec2 range_min, vec2 range_max,
        const std::function<bool(const Entity&)>& = {});

    static bool hasOverlappingSolidEntitiesInRange(vec2 range_min,
                                                   vec2 range_max);

    static OptEntity getOverlappingEntityIfExists(
        const Entity& entity, float range,
        const std::function<bool(const Entity&)>& filter = {});

    static OptEntity getMatchingFloorMarker(IsFloorMarker::Type type);
    static OptEntity getMatchingTriggerArea(IsTriggerArea::Type type);

    // TODO :INFRA: i think this is slower because we are doing "outside
    // mesh" as outside we should probably have just make some tiles for
    // inside the map
    // ('.' on map for example) and use those to mark where people can walk
    // and where they cant static bool isWalkable_impl(const vec2& pos) {
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

    static void invalidatePathCacheLocation(vec2 pos);
    static void invalidatePathCache();
    static bool isWalkable(vec2 pos);

    // each target get and path find runs through all entities
    // so this will just get slower and slower over time
    static bool isWalkableRawEntities(const vec2& pos);

   private:
    template<typename T>
    static std::vector<RefEntity> getAllWithComponent() {
        std::vector<RefEntity> matching;
        for (const auto& e : get_entities()) {
            if (!e) continue;
            if (e->has<T>()) matching.push_back(*e);
        }
        return matching;
    }

    template<typename T>
    static OptEntity getFirstWithComponent() {
        for (const auto& e : get_entities()) {
            if (!e) continue;
            if (e->has<T>()) return *e;
        }
        return {};
    }

    static std::vector<RefEntity> getAllWithType(const EntityType& type);

    static OptEntity getFirstOfType(EntityType type) {
        for (const auto& e : get_entities()) {
            if (!e) continue;
            if (check_type(*e, type)) return *e;
        }
        return {};
    }
};
