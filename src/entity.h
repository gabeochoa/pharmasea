
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

static std::atomic_int ITEM_ID_GEN = 0;
struct Item {
    int id;
    float item_size = TILESIZE / 2;
    Color color;
    vec3 raw_position;
    vec3 position;
    bool unpacked = false;

    Item(vec3 p, Color c) : id(ITEM_ID_GEN++), color(c), raw_position(p) {
        this->position = this->snap_position();
    }
    Item(vec2 p, Color c)
        : id(ITEM_ID_GEN++), color(c), raw_position({p.x, 0, p.y}) {
        this->position = this->snap_position();
    }

    virtual ~Item(){}

    vec3 snap_position() const { return vec::snap(this->raw_position); }

    virtual bool is_collidable() { return unpacked; }

    virtual void render() const {
        DrawCube(position, this->size().x, this->size().y, this->size().z,
                 this->color);
    }

    virtual void update_position(const vec3& p) {
        this->position = vec::snap(p);
    }

    virtual BoundingBox bounds() const {
        return get_bounds(this->position, this->size() / 2.0f);
    }

    virtual vec3 size() const {
        return (vec3){item_size, item_size, item_size};
    }

    virtual BoundingBox raw_bounds() const {
        return get_bounds(this->raw_position, this->size());
    }

    virtual bool collides(BoundingBox b) const {
        return CheckCollisionBoxes(this->bounds(), b);
    }
};
static std::vector<std::shared_ptr<Item>> items_DO_NOT_USE;

static std::atomic_int ENTITY_ID_GEN = 0;
struct Entity {

    enum FrontFaceDirection{
        FORWARD = 0, // 0 degrees
        RIGHT = 90,  // 90 degrees
        BACK = 180,  // 180 degrees
        LEFT = 270   // 270 degrees
    };

    int id;
    vec3 raw_position;
    vec3 prev_position;
    vec3 position;
    Color face_color;
    Color base_color;
    bool cleanup = false;
    FrontFaceDirection face_direction = FrontFaceDirection::FORWARD;
    std::shared_ptr<Item> held_item = nullptr;

    Entity(vec3 p, Color face_color_in, Color base_color_in) : id(ENTITY_ID_GEN++), raw_position(p), face_color(face_color_in), base_color(base_color_in) {
        this->position = this->snap_position();
    }

    Entity(vec2 p, Color face_color_in, Color base_color_in)
        : id(ENTITY_ID_GEN++), raw_position({p.x, 0, p.y}), face_color(face_color_in), base_color(base_color_in) {
        this->position = this->snap_position();
    }

    Entity(vec3 p, Color c) : id(ENTITY_ID_GEN++), raw_position(p), face_color(c), base_color(c) {
        this->position = this->snap_position();
    }

    Entity(vec2 p, Color c)
        : id(ENTITY_ID_GEN++), raw_position({ p.x, 0, p.y }), face_color(c), base_color(c) {
        this->position = this->snap_position();
    }

    virtual ~Entity() {}

    virtual BoundingBox bounds() const {
        return get_bounds(this->position, this->size() / 2.0f);
    }

    virtual vec3 size() const { return (vec3){TILESIZE, TILESIZE, TILESIZE}; }

    virtual BoundingBox raw_bounds() const {
        return get_bounds(this->raw_position, this->size());
    }

    virtual bool collides(BoundingBox b) const {
        return CheckCollisionBoxes(this->bounds(), b);
    }

    virtual void render() const {
        // DrawCube(this->position, this->size().x, this->size().y,
        // this->size().z,
        //          this->color);
        DrawCubeCustom(this->raw_position, this->size().x, this->size().y,
                 this->size().z, static_cast<float>(face_direction), this->face_color, this->base_color);
        DrawBoundingBox(this->bounds(), MAROON);
        // DrawBoundingBox(this->raw_bounds(), PINK);
    }

    vec3 snap_position() const { return vec::snap(this->raw_position); }

    virtual void update(float) {
        if (this->is_snappable()) {
            this->position = this->snap_position();
        }
        else {
            this->position = this->raw_position;
        }
        
        if (held_item != nullptr) {
            auto new_pos = this->position;
            if (this->face_direction == FrontFaceDirection::FORWARD) {
                new_pos.z += TILESIZE;
            }
            else if (this->face_direction == FrontFaceDirection::RIGHT) {
                new_pos.x += TILESIZE;
            }
            else if (this->face_direction == FrontFaceDirection::BACK) {
                new_pos.z -= TILESIZE;
            }
            else if (this->face_direction == FrontFaceDirection::LEFT) {
                new_pos.x -= TILESIZE;
            }
            
            held_item->update_position(new_pos);
        }
    }

    virtual bool is_collidable() { return true; }
    virtual bool is_snappable() { return false; }
};

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
