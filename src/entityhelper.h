
#pragma once

#include <thread>

#include "assert.h"
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
#include "item.h"

// TODO eventually move to system manager but for now has to be in here
// to prevent circular includes

// return true if the item has collision and is currently collidable
[[nodiscard]] inline bool is_collidable(std::shared_ptr<Entity> entity) {
    if (!entity) return false;

    // by default we disable collisions when you are holding something
    // since its generally inside your bounding box
    if (entity->has<CanBeHeld>()) {
        return !entity->get<CanBeHeld>().is_held();
    }

    if (entity->has<IsSolid>()) {
        return true;
    }

    // if you are a ghost player and are currently not a ghost
    // then you are collidable
    if (entity->has<CanBeGhostPlayer>()) {
        return entity->get<CanBeGhostPlayer>().is_not_ghost();
    }
    return false;
}

typedef std::vector<std::shared_ptr<Entity>> Entities;
static Entities client_entities_DO_NOT_USE;
static Entities server_entities_DO_NOT_USE;
static std::map<vec2, bool> cache_is_walkable;

struct EntityHelper {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

    static Entities& get_entities() {
        if (is_server()) {
            return server_entities_DO_NOT_USE;
        }
        // Right now we only have server/client thread, but in the future if we
        // have more then we should check these

        // auto client_thread_id =
        // GLOBALS.get_or_default("client_thread_id", std::thread::id());
        return client_entities_DO_NOT_USE;
    }

    static void addEntity(std::shared_ptr<Entity> e) {
        get_entities().push_back(e);
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

    static void removeEntity(std::shared_ptr<Entity> e) {
        // if (e->add_to_navmesh()) {
        // auto nav = GLOBALS.get_ptr<NavMesh>("navmesh");
        // nav->removeEntity(e->id);
        // cache_is_walkable.clear();
        // }

        auto entities = get_entities();

        auto it = entities.begin();
        while (it != get_entities().end()) {
            if ((*it)->id == e->id) {
                entities.erase(it);
                continue;
            }
            it++;
        }
    }

    // static Polygon getPolyForEntity(std::shared_ptr<Entity> e) {
    // vec2 pos = vec::to2(e->snap_position());
    // Rectangle rect = {
    // pos.x,
    // pos.y,
    // TILESIZE,
    // TILESIZE,
    // };
    // return Polygon(rect);
    // }

    static void cleanup() {
        // Cleanup entities marked cleanup
        auto entities = get_entities();

        auto it = entities.begin();
        while (it != entities.end()) {
            if ((*it)->cleanup) {
                entities.erase(it);
                continue;
            }
            it++;
        }
    }

    enum ForEachFlow {
        NormalFlow = 0,
        Continue = 1,
        Break = 2,
    };

    static void forEachEntity(
        std::function<ForEachFlow(std::shared_ptr<Entity>)> cb) {
        TRACY_ZONE_SCOPED;
        for (auto e : get_entities()) {
            if (!e) continue;
            auto fef = cb(e);
            if (fef == 1) continue;
            if (fef == 2) break;
        }
    }

    template<typename T>
    static constexpr std::vector<std::shared_ptr<T>> getEntitiesInRange(
        vec2 pos, float range) {
        std::vector<std::shared_ptr<T>> matching;
        for (auto& e : get_entities()) {
            auto s = dynamic_pointer_cast<T>(e);
            if (!s) continue;
            if (vec::distance(pos, e->get<Transform>().as2()) < range) {
                matching.push_back(s);
            }
        }
        return matching;
    }

    template<typename T>
    static std::shared_ptr<T> getMatchingEntityInFront(
        vec2 pos,                                       //
        float range,                                    //
        Transform::FrontFaceDirection direction,        //
        std::function<bool(std::shared_ptr<T>)> filter  //
    ) {
        TRACY_ZONE_SCOPED;
        VALIDATE(range > 0,
                 fmt::format("range has to be positive but was {}", range));

        int cur_step = 0;
        while (cur_step <= range) {
            auto tile =
                Transform::tile_infront_given_pos(pos, cur_step, direction);

            for (auto& e : get_entities()) {
                auto current_entity = dynamic_pointer_cast<T>(e);
                if (!current_entity) continue;
                if (!filter(current_entity)) continue;

                // all entitites should have transforms but just in case
                if (current_entity->template is_missing<Transform>()) {
                    log_warn("component {} is missing transform",
                             current_entity->id);
                    continue;
                }

                Transform& transform =
                    current_entity->template get<Transform>();

                float cur_dist = vec::distance(transform.as2(), tile);
                // outside reach
                if (abs(cur_dist) > 1) continue;
                // this is behind us
                if (cur_dist < 0) continue;

                // TODO add a snap_as2() function to transform
                if (vec::to2(transform.snap_position()) == vec::snap(tile)) {
                    return current_entity;
                }
            }
            cur_step++;
        }
        return {};
    }

    template<typename T>
    static constexpr std::shared_ptr<T> getClosestMatchingEntity(
        vec2 pos, float range, std::function<bool(std::shared_ptr<T>)> filter) {
        float best_distance = range;
        std::shared_ptr<T> best_so_far;
        for (auto& e : get_entities()) {
            auto s = dynamic_pointer_cast<T>(e);
            if (!s) continue;
            if (!filter(s)) continue;
            float d = vec::distance(pos, e->get<Transform>().as2());
            if (d > range) continue;
            if (d < best_distance) {
                best_so_far = s;
                best_distance = d;
            }
        }
        return best_so_far;
    }

    static std::shared_ptr<Entity> getClosestMatchingFurniture(
        const Transform& transform, float range,
        std::function<bool(std::shared_ptr<Furniture>)> filter) {
        return EntityHelper::getMatchingEntityInFront<Furniture>(
            transform.as2(), range, transform.face_direction(), filter);
    }

    template<typename T>
    static std::shared_ptr<Entity> getClosestWithComponent(
        const std::shared_ptr<Entity>& entity, float range) {
        const Transform& transform = entity->get<Transform>();
        return EntityHelper::getClosestMatchingEntity<Furniture>(
            transform.as2(), range, [](const std::shared_ptr<Entity> entity) {
                return entity->has<T>();
            });
    }

    // TODO i think this is slower because we are doing "outside mesh" as
    // outside we should probably have just make some tiles for inside the map
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

    // TODO need to invalidate any current valid paths
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
        auto bounds = get_bounds({pos.x, 0.f, pos.y}, {TILESIZE});
        bool hit_impassible_entity = false;
        forEachEntity([&](auto entity) {
            if (!is_collidable(entity)) return ForEachFlow::Continue;

            if (entity->template get<Transform>().collides(bounds)) {
                hit_impassible_entity = true;
                return ForEachFlow::Break;
            }
            return ForEachFlow::Continue;
        });
        return !hit_impassible_entity;
    }

#pragma clang diagnostic pop
};
