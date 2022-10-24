#pragma once

#include "external_include.h"
//
#include "globals.h"
#include "modellibrary.h"
#include "random.h"
#include "raylib.h"
#include "ui_color.h"
#include "vec_util.h"

static std::atomic_int ITEM_ID_GEN = 0;
struct Item {
    enum HeldBy { NONE, PLAYER, FURNITURE, CUSTOMER };

    int id;
    float item_size = TILESIZE / 2;
    raylib::Color color;
    raylib::vec3 raw_position;
    raylib::vec3 position;
    bool unpacked = false;
    HeldBy held_by = NONE;

    Item(raylib::vec3 p, raylib::Color c)
        : id(ITEM_ID_GEN++), color(c), raw_position(p) {
        this->position = this->snap_position();
    }
    Item(raylib::vec2 p, raylib::Color c)
        : id(ITEM_ID_GEN++), color(c), raw_position({p.x, 0, p.y}) {
        this->position = this->snap_position();
    }

    virtual ~Item() {}

    raylib::vec3 snap_position() const { return vec::snap(this->raw_position); }

    virtual bool is_collidable() { return unpacked; }

    virtual void render() const {
        std::optional<raylib::Model> m = this->model();
        if (m.has_value()) {
            raylib::DrawModel(m.value(), this->position, this->size().x * 0.25f,
                              ui::color::tan_brown);
        } else {
            raylib::DrawCube(position, this->size().x, this->size().y,
                             this->size().z, this->color);
        }
    }

    virtual void update_position(const raylib::vec3& p) { this->position = p; }

    virtual raylib::BoundingBox bounds() const {
        return get_bounds(this->position, this->size() / 2.0f);
    }

    virtual raylib::vec3 size() const {
        return (raylib::vec3){item_size, item_size, item_size};
    }

    virtual raylib::BoundingBox raw_bounds() const {
        return get_bounds(this->raw_position, this->size());
    }

    virtual bool collides(raylib::BoundingBox b) const {
        return CheckCollisionBoxes(this->bounds(), b);
    }

    virtual std::optional<raylib::Model> model() const { return {}; }
};
static std::vector<std::shared_ptr<Item>> items_DO_NOT_USE;

struct Bag : public Item {
    Bag(raylib::vec3 p, raylib::Color c) : Item(p, c) {}
    Bag(raylib::vec2 p, raylib::Color c) : Item(p, c) {}

    virtual std::optional<raylib::Model> model() const override {
        return ModelLibrary::get().get("bag");
    }
};
