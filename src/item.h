#pragma once

#include "external_include.h"
//
#include "globals.h"
#include "random.h"
#include "raylib.h"
#include "vec_util.h"

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

    virtual ~Item() {}

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
