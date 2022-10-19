
#pragma once

#include "external_include.h"
//
#include "globals.h"
#include "navmesh.h"

//
#include "entity.h"
#include "item.h"

static std::vector<std::shared_ptr<Entity>> entities_DO_NOT_USE;
static std::map<vec2, bool> cache_is_walkable;

struct EntityHelper {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    static void addEntity(std::shared_ptr<Entity> e) {
        entities_DO_NOT_USE.push_back(e);
        if (!e->add_to_navmesh()) {
            return;
        }
        auto nav = GLOBALS.get_ptr<NavMesh>("navmesh");
        // Note: addShape merges shapes next to each other
        //      this reduces the amount of loops overall

        // nav->addShape(getPolyForEntity(e));
        nav->addEntity(e->id, getPolyForEntity(e));
        cache_is_walkable.clear();
    }

    static void removeEntity(std::shared_ptr<Entity> e) {
        if (e->add_to_navmesh()) {
            auto nav = GLOBALS.get_ptr<NavMesh>("navmesh");
            nav->removeEntity(e->id);
            cache_is_walkable.clear();
        }

        auto it = entities_DO_NOT_USE.begin();
        while (it != entities_DO_NOT_USE.end()) {
            if ((*it)->id == e->id) {
                entities_DO_NOT_USE.erase(it);
                continue;
            }
            it++;
        }
    }

    static Polygon getPolyForEntity(std::shared_ptr<Entity> e) {
        vec2 pos = vec::to2(e->snap_position());
        Rectangle rect = {
            pos.x,
            pos.y,
            TILESIZE,
            TILESIZE,
        };
        return Polygon(rect);
    }

    static void addItem(std::shared_ptr<Item> item) {
        items_DO_NOT_USE.push_back(item);
    }
    static void cleanup() {
        // Cleanup entities marked cleanup
        auto it = entities_DO_NOT_USE.begin();
        while (it != entities_DO_NOT_USE.end()) {
            if ((*it)->cleanup) {
                entities_DO_NOT_USE.erase(it);
                continue;
            }
            it++;
        }
    }

    enum ForEachFlow {
        None = 0,
        Continue = 1,
        Break = 2,
    };

    static void forEachEntity(
        std::function<ForEachFlow(std::shared_ptr<Entity>)> cb) {
        for (auto e : entities_DO_NOT_USE) {
            if (!e) continue;
            auto fef = cb(e);
            if (fef == 1) continue;
            if (fef == 2) break;
        }
    }

    static void forEachItem(
        std::function<ForEachFlow(std::shared_ptr<Item>)> cb) {
        for (auto e : items_DO_NOT_USE) {
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
        for (auto& e : entities_DO_NOT_USE) {
            auto s = dynamic_pointer_cast<T>(e);
            if (!s) continue;
            if (vec::distance(pos, vec::to2(e->position)) < range) {
                matching.push_back(s);
            }
        }
        return matching;
    }

    template<typename T>
    static std::shared_ptr<T> getMatchingEntityInFront(
        vec2 pos, float range, Entity::FrontFaceDirection direction,
        std::function<bool(std::shared_ptr<T>)> filter) {
        std::vector<vec2> steps;
        // TODO fix this iterator up
        for (int i = 0; i < static_cast<int>(range); i++) {
            steps.push_back(
                vec::snap(Entity::tile_infront_given_pos(pos, i, direction)));
        }

        for (auto& e : entities_DO_NOT_USE) {
            auto s = dynamic_pointer_cast<T>(e);
            if (!s) continue;
            if (!filter(s)) continue;
            float d = vec::distance(pos, vec::to2(s->position));
            if (d > range) continue;
            for (auto step : steps) {
                d = vec::distance(step, vec::snap(vec::to2(s->position)));
                if (abs(d) <= 1.f) return s;
            }
            return s;
        }
        return {};
    }

    template<typename T>
    static constexpr std::shared_ptr<T> getClosestMatchingEntity(
        vec2 pos, float range, std::function<bool(std::shared_ptr<T>)> filter) {
        float best_distance = range;
        std::shared_ptr<T> best_so_far;
        for (auto& e : entities_DO_NOT_USE) {
            auto s = dynamic_pointer_cast<T>(e);
            if (!s) continue;
            if (!filter(s)) continue;
            float d = vec::distance(pos, vec::to2(e->position));
            if (d > range) continue;
            if (d < best_distance) {
                best_so_far = s;
                best_distance = d;
            }
        }
        return best_so_far;
    }

    static std::shared_ptr<Item> getClosestMatchingItem(vec2 pos, float range) {
        float best_distance = range;
        std::shared_ptr<Item> best_so_far;
        for (auto& i : items_DO_NOT_USE) {
            float d = vec::distance(pos, vec::to2(i->position));
            if (d > range) continue;
            if (d < best_distance) {
                best_so_far = i;
                best_distance = d;
            }
        }
        return best_so_far;
    }

    // TODO i think this is slower because we are doing "outside mesh" as
    // outside we should probably have just make some tiles for inside the map
    // ('.' on map for example) and use those to mark where people can walk and
    // where they cant
    static bool isWalkable_impl(const vec2& pos) {
        auto nav = GLOBALS.get_ptr<NavMesh>("navmesh");
        if (!nav) {
            return true;
        }

        for (auto kv : nav->entityShapes) {
            auto s = kv.second;
            if (s.inside(pos)) return false;
        }
        return true;
    }

    static bool isWalkable(const vec2& pos) {
        if (!cache_is_walkable.contains(pos)) {
            cache_is_walkable[pos] = isWalkableRawEntities(pos);
        }
        return cache_is_walkable[pos];
    }

    // each target get and path find runs through all entities
    // so this will just get slower and slower over time
    static bool isWalkableRawEntities(const vec2& pos) {
        auto bounds = get_bounds({pos.x, 0.f, pos.y}, {TILESIZE});
        bool hit_impassible_entity = false;
        forEachEntity([&](auto entity) {
            if (entity->is_collidable() && entity->collides(bounds)) {
                hit_impassible_entity = true;
                return ForEachFlow::Break;
            }
            return ForEachFlow::Continue;
        });
        return !hit_impassible_entity;
    }

#pragma clang diagnostic pop
};
