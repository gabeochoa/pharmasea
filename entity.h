
#pragma once

#include "astar.h"
#include "external_include.h"
#include "globals.h"
#include "random.h"
#include "raylib.h"
#include "vec_util.h"

BoundingBox get_bounds(vec3 position) {
    const float half_tile = TILESIZE / 2.f;
    return {(vec3){
                position.x - half_tile,
                position.y - half_tile,
                position.z - half_tile,
            },
            (vec3){
                position.x + half_tile,
                position.y + half_tile,
                position.z + half_tile,
            }};
}

static std::atomic_int ENTITY_ID_GEN = 0;
struct Entity {
    int id;
    vec3 position;
    Color color;
    bool cleanup = false;

    Entity(vec3 p, Color c) : id(ENTITY_ID_GEN++), position(p), color(c) {}
   Entity(vec2 p, Color c)
        : id(ENTITY_ID_GEN++), position({p.x, 0, p.y}), color(c) {}
    virtual ~Entity() {}

    virtual BoundingBox bounds() const { return get_bounds(this->position); }

    virtual bool collides(BoundingBox b) const {
        return CheckCollisionBoxes(this->bounds(), b);
    }

    virtual void render() const {
        DrawCube(position, TILESIZE, TILESIZE, TILESIZE, this->color);
        DrawBoundingBox(this->bounds(), MAROON);
    }

    vec3 snap_position() { return vec::snap(this->position); }

    virtual void update(float) {}
};
static std::vector<std::shared_ptr<Entity>> entities_DO_NOT_USE;

struct EntityHelper {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    static void addEntity(std::shared_ptr<Entity> e) {
        entities_DO_NOT_USE.push_back(e);
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
#pragma clang diagnostic pop
};
