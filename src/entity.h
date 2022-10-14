
#pragma once

#include "external_include.h"
//
#include "astar.h"
#include "globals.h"
#include "item.h"
#include "random.h"
#include "raylib.h"
#include "vec_util.h"

static std::atomic_int ENTITY_ID_GEN = 0;
struct Entity {
    enum FrontFaceDirection {
        FORWARD = 0,  // 0 degrees
        RIGHT = 90,   // 90 degrees
        BACK = 180,   // 180 degrees
        LEFT = 270,    // 270 degrees
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

    Entity(vec3 p, Color face_color_in, Color base_color_in)
        : id(ENTITY_ID_GEN++),
          raw_position(p),
          face_color(face_color_in),
          base_color(base_color_in) {
        this->position = this->snap_position();
    }

    Entity(vec2 p, Color face_color_in, Color base_color_in)
        : id(ENTITY_ID_GEN++),
          raw_position({p.x, 0, p.y}),
          face_color(face_color_in),
          base_color(base_color_in) {
        this->position = this->snap_position();
    }

    Entity(vec3 p, Color c)
        : id(ENTITY_ID_GEN++), raw_position(p), face_color(c), base_color(c) {
        this->position = this->snap_position();
    }

    Entity(vec2 p, Color c)
        : id(ENTITY_ID_GEN++),
          raw_position({p.x, 0, p.y}),
          face_color(c),
          base_color(c) {
        this->position = this->snap_position();
    }

    virtual ~Entity() {}

    virtual BoundingBox bounds() const {
        return get_bounds(this->position, this->size() / 2.0f);
    }

    virtual void update_position(const vec3& p) {
        this->raw_position = p;
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
                       this->size().z, static_cast<float>(face_direction),
                       this->face_color, this->base_color);
        DrawBoundingBox(this->bounds(), MAROON);
        // DrawBoundingBox(this->raw_bounds(), PINK);
    }

    vec3 snap_position() const { return vec::snap(this->raw_position); }

    void rotate_facing_clockwise(){
        this->face_direction = static_cast<FrontFaceDirection>((this->face_direction + 90) % 360);
    }

    virtual void update(float) {
        if (this->is_snappable()) {
            this->position = this->snap_position();
        } else {
            this->position = this->raw_position;
        }

        if (held_item != nullptr) {
            auto new_pos = this->position;
            if (this->face_direction == FrontFaceDirection::FORWARD) {
                new_pos.z += TILESIZE;
            } else if (this->face_direction == FrontFaceDirection::RIGHT) {
                new_pos.x += TILESIZE;
            } else if (this->face_direction == FrontFaceDirection::BACK) {
                new_pos.z -= TILESIZE;
            } else if (this->face_direction == FrontFaceDirection::LEFT) {
                new_pos.x -= TILESIZE;
            }

            held_item->update_position(new_pos);
        }
    }

    virtual bool is_collidable() { return true; }
    virtual bool is_snappable() { return false; }
};

typedef Entity::FrontFaceDirection EntityDir;
