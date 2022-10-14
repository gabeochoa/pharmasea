
#pragma once 

#include "external_include.h"
//

//
#include "entity.h"
#include "item.h"

static std::vector<std::shared_ptr<Entity>> entities_DO_NOT_USE;

struct EntityHelper {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    static void addEntity(std::shared_ptr<Entity> e) {
        entities_DO_NOT_USE.push_back(e);
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

    template <typename T>
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
        static constexpr std::shared_ptr<T> getClosestMatchingEntity(vec2 pos, float range, std::function<bool(std::shared_ptr<T>)> filter){
        float best_distance = range;
        std::shared_ptr<T> best_so_far;
        for (auto& e : entities_DO_NOT_USE) {
            auto s = dynamic_pointer_cast<T>(e);
            if (!s) continue;
            if (!filter(s)) continue;
            float d = vec::distance(pos, vec::to2(e->position));
            if(d > range) continue;
            if(d < best_distance){
                best_so_far = s;
                best_distance = d;
            }
        }
        return best_so_far;
    }

   static std::shared_ptr<Item> getClosestMatchingItem(vec2 pos, float range){
        float best_distance = range;
        std::shared_ptr<Item> best_so_far;
        for (auto& i : items_DO_NOT_USE) {
            float d = vec::distance(pos, vec::to2(i->position));
            if(d > range) continue;
            if(d < best_distance){
                best_so_far = i;
                best_distance = d;
            }
        }
        return best_so_far;
    }

    // TODO replace with navmesh
    // each target get and path find runs through all entities
    // so this will just get slower and slower over time
    static bool isWalkable(const vec2& pos) {
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
