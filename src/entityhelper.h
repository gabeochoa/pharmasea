
#pragma once 

#include "external_include.h"
//

//
#include "entity.h"

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
