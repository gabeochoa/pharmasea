
#pragma once

#include <thread>

#include "assert.h"
#include "components/can_hold_item.h"
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

enum struct NamedEntity {
    Sophie,
};

using Entities = std::vector<std::shared_ptr<afterhours::Entity>>;
using RefEntities = std::vector<afterhours::RefEntity>;

using NamedEntities = std::map<NamedEntity, std::shared_ptr<Entity>>;

extern Entities client_entities_DO_NOT_USE;
extern Entities server_entities_DO_NOT_USE;
extern NamedEntities named_entities_DO_NOT_USE;

extern std::set<int> permanant_ids;
extern std::map<vec2, bool> cache_is_walkable;

struct EntityHelper {
    struct CreationOptions {
        bool is_permanent;
    };

    static const Entities& get_entities();
    static Entities& get_entities_for_mod();
    static RefEntities get_ref_entities();

    static Entity& createEntity();
    static Entity& createPermanentEntity();
    static Entity& createEntityWithOptions(const CreationOptions& options);

    static Entity& getNamedEntity(const NamedEntity& name);
    static OptEntity getPossibleNamedEntity(const NamedEntity& name);

    // TODO :BE: maybe return the entity id or something
    template<typename... TArgs>
    static RefEntity createItem(EntityType type, vec3 pos, TArgs... args) {
        items::make_item_type(createEntity(), type, pos,
                              std::forward<TArgs>(args)...);
        // log_info("created a new item {} {} ", e.id, e.name());
        return *(get_entities().back());
    }

    template<typename... TArgs>
    static RefEntity createPermanentItem(vec3 pos, TArgs... args) {
        items::make_item_type(createPermanentEntity(), pos,
                              std::forward<TArgs>(args)...);
        // log_info("created a new item {} {} ", e.id, e.name());
        return *(get_entities().back());
    }

    static void markIDForCleanup(int e_id);
    static void removeEntity(int e_id);
    static void cleanup();
    static void delete_all_entities_NO_REALLY_I_MEAN_ALL();
    static void delete_all_entities(bool include_permanent = false);

    // Rebuild the handle/ID mapping for the current entity list.
    // Intended for cases where entities are injected externally (e.g. network
    // deserialization assigning `client_entities_DO_NOT_USE` directly).
    static void rebuild_handle_store_for_current_entities();

    enum ForEachFlow {
        NormalFlow = 0,
        Continue = 1,
        Break = 2,
    };

    static void forEachEntity(std::function<ForEachFlow(Entity&)> cb);

    static std::vector<RefEntity> getFilteredEntitiesInRange(
        vec2 pos, float range,
        const std::function<bool(const Entity&)>& filter);

    // TODO exists as a conversion for things that need shared_ptr right now
    static std::shared_ptr<Entity> getEntityAsSharedPtr(const Entity& entity) {
        for (std::shared_ptr<Entity> current_entity : get_entities()) {
            if (entity.id == current_entity->id) return current_entity;
        }
        return {};
    }

    static std::shared_ptr<Entity> getEntityAsSharedPtr(OptEntity entity) {
        if (!entity) return {};
        const Entity& e = entity.asE();
        return getEntityAsSharedPtr(e);
    }

    static OptEntity getClosestMatchingFurniture(
        const Transform& transform, float range,
        const std::function<bool(const Entity&)>& filter);

    static OptEntity getEntityForID(EntityID id);

    // Like getEntityForID, but asserts when missing.
    static Entity& getEnforcedEntityForID(EntityID id);

    // Handle-based APIs (pointer-free, stable identifiers)
    // NOTE: These are Phase 1 building blocks; storage remains in PharmaSea.
    static EntityHandle handle_for(const Entity& e);
    static OptEntity resolve(EntityHandle h);

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

    static RefEntities getAllInRange(vec2 range_min, vec2 range_max);
    static RefEntities getAllInRangeFiltered(
        vec2 range_min, vec2 range_max,
        const std::function<bool(const Entity&)>& filter);

    static OptEntity getOverlappingEntityIfExists(
        const Entity& entity, float range,
        const std::function<bool(const Entity&)>& filter = {},
        bool include_store_entities = false);

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
    //
    static void invalidateCaches();

    static void invalidatePathCacheLocation(vec2 pos);
    static void invalidatePathCache();
    static bool isWalkable(vec2 pos);

    // each target get and path find runs through all entities
    // so this will just get slower and slower over time
    static bool isWalkableRawEntities(const vec2& pos);
};
