
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
#include "afterhours/src/core/entity_helper.h"
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

// Thread-specific EntityCollections
// Each thread can have its own collection for independent entity management
extern afterhours::EntityCollection client_collection;
extern afterhours::EntityCollection server_collection;

extern NamedEntities named_entities_DO_NOT_USE;
extern std::map<vec2, bool> cache_is_walkable;

struct EntityHelper : afterhours::EntityHelper {
    static afterhours::EntityCollection& get_current_collection() {
        if (is_server()) {
            return server_collection;
        }
        return client_collection;
    }

    // Named entity functionality
    static Entity& getNamedEntity(const NamedEntity& name);
    static OptEntity getPossibleNamedEntity(const NamedEntity& name);

    // Game-specific query methods
    static OptEntity getEntityForID(afterhours::EntityID id) {
        if (id == entity_id::INVALID) return {};
        return get_current_collection().getEntityForID(id);
    }

    // Like getEntityForID, but asserts when missing.
    static Entity& getEnforcedEntityForID(afterhours::EntityID id) {
        OptEntity opt = getEntityForID(id);
        if (!opt) {
            log_error("EntityHelper::getEnforcedEntityForID failed: {}", id);
        }
        return opt.asE();
    }

    // Pathfinding and walkability
    static void invalidateCaches();
    static void invalidatePathCache();
    static bool isWalkable(vec2 pos);
    static bool isWalkableRawEntities(const vec2& pos);

    // Creation stuff

    struct CreationOptions {
        bool is_permanent;
    };

    // Get a specific collection by thread type
    static afterhours::EntityCollection& get_server_collection() {
        return server_collection;
    }
    static afterhours::EntityCollection& get_client_collection() {
        return client_collection;
    }

    // Game-specific entity access methods
    static const Entities& get_entities() {
        return EntityHelper::get_current_collection().get_entities();
    }
    static Entities& get_entities_for_mod() {
        return EntityHelper::get_current_collection().get_entities_for_mod();
    }
    static RefEntities get_ref_entities();

    // Entity creation with game-specific options
    static Entity& createEntity();
    static Entity& createPermanentEntity();
    static Entity& createEntityWithOptions(const CreationOptions& options);

    // Item creation helpers
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

    // Cleanup and entity management
    static void markIDForCleanup(int e_id) {
        EntityHelper::get_current_collection().markIDForCleanup(e_id);
    }
    static void cleanup() { EntityHelper::get_current_collection().cleanup(); }
    static void delete_all_entities_NO_REALLY_I_MEAN_ALL() {
        EntityHelper::get_current_collection()
            .delete_all_entities_NO_REALLY_I_MEAN_ALL();
    }
    static void delete_all_entities(bool include_permanent = false) {
        EntityHelper::get_current_collection().delete_all_entities(
            include_permanent);
    }

    // Entity iteration
    static void forEachEntity(
        const std::function<
            afterhours::EntityHelper::ForEachFlow(afterhours::Entity&)>& cb);

    // Entity query methods
    static std::vector<RefEntity> getFilteredEntitiesInRange(
        vec2 pos, float range,
        const std::function<bool(const Entity&)>& filter);
};
