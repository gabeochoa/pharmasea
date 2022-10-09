
#pragma once

#include "astar.h"
#include "external_include.h"
#include "globals.h"
#include "random.h"
#include "raylib.h"
#include "vec_util.h"

BoundingBox get_bounds(vec3 position, vec3 size) {
    return {(vec3){
                position.x - size.x / 2,
                position.y - size.y / 2,
                position.z - size.z / 2,
            },
            (vec3){
                position.x + size.x / 2,
                position.y + size.y / 2,
                position.z + size.z / 2,
            }};
}

static std::atomic_int ENTITY_ID_GEN = 0;
struct Entity {
    int id;
    vec3 raw_position;
    vec3 position;
    Color color;
    bool cleanup = false;

    Entity(vec3 p, Color c) : id(ENTITY_ID_GEN++), raw_position(p), color(c) {
        this->position = this->snap_position();
    }
    Entity(vec2 p, Color c)
        : id(ENTITY_ID_GEN++), raw_position({p.x, 0, p.y}), color(c) {
        this->position = this->snap_position();
    }
    virtual ~Entity() {}

    virtual BoundingBox bounds() const {
        return get_bounds(this->position, this->size() / 2.0f );
    }

    virtual vec3 size() const { return (vec3){TILESIZE, TILESIZE, TILESIZE}; }

    virtual BoundingBox raw_bounds() const {
        return get_bounds(this->raw_position, this->size());
    }

    virtual bool collides(BoundingBox b) const {
        return CheckCollisionBoxes(this->bounds(), b);
    }

    virtual void render() const {
        //DrawCube(this->position, this->size().x, this->size().y, this->size().z,
        //         this->color);
        DrawCube(this->raw_position, this->size().x, this->size().y, this->size().z, this->color); 
        DrawBoundingBox(this->bounds(), MAROON);
        //DrawBoundingBox(this->raw_bounds(), PINK);
    }

    vec3 snap_position() const { return vec::snap(this->raw_position); }

    virtual void update(float) {
        this->position = this->snap_position();
    }
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

    // TODO replace with navmesh
    // each target get and path find runs through all entities
    // so this will just get slower and slower over time
    static bool isWalkable(const vec2& pos) {
        auto bounds = get_bounds({pos.x, 0.f, pos.y}, {TILESIZE});
        bool hit_impassible_entity = false;
        forEachEntity([&](auto entity) {
            if (entity->collides(bounds)) {
                hit_impassible_entity = true;
                return ForEachFlow::Break;
            }
            return ForEachFlow::Continue;
        });
        return !hit_impassible_entity;
    }

#pragma clang diagnostic pop
};
