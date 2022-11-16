
#pragma once

#include "external_include.h"
//
#include <map>

#include "astar.h"
#include "globals.h"
#include "is_server.h"
#include "item.h"
#include "item_helper.h"
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
    bool is_held = false;
    FrontFaceDirection face_direction = FrontFaceDirection::FORWARD;
    std::shared_ptr<Item> held_item = nullptr;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.value4b(id);
        s.object(raw_position);
        s.object(prev_position);
        s.object(position);
        s.object(pushed_force);
        s.object(face_color);
        s.object(base_color);
        s.value1b(cleanup);
        s.value1b(is_highlighted);
        s.value1b(is_held);
        s.value4b(face_direction);
        s.object(held_item);
    }

   public:
    // TODO is there a way for us to make this private except for bitsery?
    Entity() {}

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

    virtual void render_debug_mode() const {
        DrawBoundingBox(this->bounds(), MAROON);
    }

    virtual void render_highlighted() const {
        Color f = ui::color::getHighlighted(this->face_color);
        Color b = ui::color::getHighlighted(this->base_color);
        DrawCubeCustom(this->raw_position, this->size().x, this->size().y,
                       this->size().z, FrontFaceDirectionMap.at(face_direction),
                       f, b);
    }

    virtual void render_normal() const {
        if (this->is_highlighted) {
            render_highlighted();
            return;
        }

        DrawCubeCustom(this->raw_position, this->size().x, this->size().y,
                       this->size().z, FrontFaceDirectionMap.at(face_direction),
                       this->face_color, this->base_color);
    }

    virtual void render() const final {
        const auto debug_mode_on =
            GLOBALS.get_or_default<bool>("debug_ui_enabled", false);

        if (!debug_mode_on) {
            render_normal();
        }

        if (debug_mode_on) {
            render_debug_mode();
        }
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
            tile.y = ceil(tile.y);
        }
        if (direction & BACK) {
            tile.y -= distance * TILESIZE;
            tile.y = floor(tile.y);
        }

        if (direction & RIGHT) {
            tile.x += distance * TILESIZE;
            tile.x = ceil(tile.x);
        }
        if (direction & LEFT) {
            tile.x -= distance * TILESIZE;
            tile.x = floor(tile.x);
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

    virtual void on_pickup() { this->is_held = true; }

    virtual void on_drop(vec3 location) {
        this->is_held = false;
        this->update_position(vec::snap(location));
    }

    virtual bool is_collidable() { return !is_held; }
    virtual bool is_snappable() { return false; }
    virtual bool add_to_navmesh() { return false; }
    virtual bool can_place_item_into() { return false; }
};

typedef Entity::FrontFaceDirection EntityDir;
