
#pragma once

#include "external_include.h"
//
#include <map>

#include "astar.h"
#include "globals.h"
#include "item.h"
#include "random.h"
#include "raylib.h"
#include "util.h"
#include "vec_util.h"

static std::atomic_int ENTITY_ID_GEN = 0;
struct Entity {
    enum FrontFaceDirection {
        FORWARD = 0x1,
        RIGHT = 0x2,
        BACK = 0x4,
        LEFT = 0x8
    };

    const std::map<FrontFaceDirection, float> FrontFaceDirectionMap{
        {FORWARD, 0.0f},        {FORWARD | RIGHT, 45.0f}, {RIGHT, 90.0f},
        {BACK | RIGHT, 135.0f}, {BACK, 180.0f},           {BACK | LEFT, 225.0f},
        {LEFT, 270.0f},         {FORWARD | LEFT, 315.0f}};

    const std::map<int, FrontFaceDirection> DirectionToFrontFaceMap{
        {0, FORWARD},        {45, FORWARD | RIGHT}, {90, RIGHT},
        {135, BACK | RIGHT}, {180, BACK},           {225, BACK | LEFT},
        {270, LEFT},         {315, FORWARD | LEFT}};

    FrontFaceDirection offsetFaceDirection(FrontFaceDirection startingDirection,
                                           float offset) const {
        const auto degreesOffset =
            static_cast<int>(FrontFaceDirectionMap.at(startingDirection) +
                             static_cast<int>(offset));
        return DirectionToFrontFaceMap.at(degreesOffset % 360);
    }

    int id;
    vec3 raw_position;
    vec3 prev_position;
    vec3 position;
    vec3 pushed_force{0.0, 0.0, 0.0};
    Color face_color;
    Color base_color;
    bool cleanup = false;
    bool is_highlighted = false;
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

    virtual void update_position(const vec3& p) { this->raw_position = p; }

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

        if (this->is_highlighted) {
            Color f = ui::color::getHighlighted(this->face_color);
            Color b = ui::color::getHighlighted(this->base_color);
            DrawCubeCustom(this->raw_position, this->size().x, this->size().y,
                           this->size().z,
                           FrontFaceDirectionMap.at(face_direction), f, b);
        } else {
            DrawCubeCustom(this->raw_position, this->size().x, this->size().y,
                           this->size().z,
                           FrontFaceDirectionMap.at(face_direction),
                           this->face_color, this->base_color);
        }
        DrawBoundingBox(this->bounds(), MAROON);
    }

    vec3 snap_position() const { return vec::snap(this->raw_position); }

    void rotate_facing_clockwise() {
        this->face_direction = offsetFaceDirection(this->face_direction, 90);
    }

    virtual void update_held_item_position() {
        if (held_item != nullptr) {
            auto new_pos = this->position;
            if (this->face_direction & FrontFaceDirection::FORWARD) {
                new_pos.z += TILESIZE;
            }
            if (this->face_direction & FrontFaceDirection::RIGHT) {
                new_pos.x += TILESIZE;
            }
            if (this->face_direction & FrontFaceDirection::BACK) {
                new_pos.z -= TILESIZE;
            }
            if (this->face_direction & FrontFaceDirection::LEFT) {
                new_pos.x -= TILESIZE;
            }

            held_item->update_position(new_pos);
        }
    }

    virtual void update(float) {
        is_highlighted = false;
        if (this->is_snappable()) {
            this->position = this->snap_position();
        } else {
            this->position = this->raw_position;
        }
        update_held_item_position();
    }

    virtual vec2 get_heading() {
        const float target_facing_ang =
            util::deg2rad(FrontFaceDirectionMap.at(this->face_direction));
        return vec2{
            cosf(target_facing_ang),
            sinf(target_facing_ang),
        };
    }

    static vec2 tile_infront_given_pos(vec2 tile, int distance,
                                       FrontFaceDirection direction) {
        if (direction & FORWARD) {
            tile.y += distance * TILESIZE;
        }
        if (direction & BACK) {
            tile.y -= distance * TILESIZE;
        }

        if (direction & RIGHT) {
            tile.x -= distance * TILESIZE;
        }
        if (direction & LEFT) {
            tile.x += distance * TILESIZE;
        }
        return tile;
    }

    virtual vec2 tile_infront(int distance) {
        vec2 tile = vec::to2(this->snap_position());
        return tile_infront_given_pos(tile, distance, this->face_direction);
    }

    virtual void turn_to_face_entity(Entity* target) {
        if (!target) return;

        // dot product visualizer https://www.falstad.com/dotproduct/

        // the angle between two vecs is
        // @ = arccos(  (a dot b) / ( |a| * |b| ) )

        // first get the headings and normalise so |x| = 1
        const vec2 my_heading = vec::norm(this->get_heading());
        const vec2 tar_heading = vec::norm(target->get_heading());
        // dp = ( (a dot b )/ 1)
        float dot_product = vec::dot2(my_heading, tar_heading);
        // arccos(dp)
        float theta_rad = acosf(dot_product);
        float theta_deg = util::rad2deg(theta_rad);
        int turn_degrees = (180 - (int) theta_deg) % 360;
        // TODO fix this 
        (void) turn_degrees;
        /*
        if (turn_degrees > 0 && turn_degrees <= 45) {
            this->face_direction = static_cast<FrontFaceDirection>(0);
        } else if (turn_degrees > 45 && turn_degrees <= 135) {
            this->face_direction = static_cast<FrontFaceDirection>(90);
        } else if (turn_degrees > 135 && turn_degrees <= 225) {
            this->face_direction = static_cast<FrontFaceDirection>(180);
        } else if (turn_degrees > 225) {
            this->face_direction = static_cast<FrontFaceDirection>(270);
        }
        */
    }

    virtual bool is_collidable() { return true; }
    virtual bool is_snappable() { return false; }
    virtual bool add_to_navmesh() { return false; }
    virtual bool can_place_item_into() { return false; }
};

typedef Entity::FrontFaceDirection EntityDir;
