#pragma once

#include "external_include.h"
//
#include "globals.h"
#include "model_library.h"
#include "random.h"
#include "raylib.h"
#include "ui_color.h"
#include "vec_util.h"

static std::atomic_int ITEM_ID_GEN = 0;
struct Item {
    enum HeldBy { NONE, ITEM, PLAYER, FURNITURE, CUSTOMER };

    int id;
    float item_size = TILESIZE / 2;
    Color color;
    vec3 raw_position;
    vec3 position;
    bool cleanup = false;
    HeldBy held_by = NONE;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.value4b(id);
        s.value4b(item_size);
        s.object(color);
        s.object(raw_position);
        s.object(position);
        s.value1b(cleanup);
        s.value4b(held_by);
    }

   protected:
    Item() {}

   public:
    Item(vec3 p, Color c) : id(ITEM_ID_GEN++), color(c), raw_position(p) {
        this->position = this->snap_position();
    }
    Item(vec2 p, Color c)
        : id(ITEM_ID_GEN++), color(c), raw_position({p.x, 0, p.y}) {
        this->position = this->snap_position();
    }

    virtual ~Item() {}

    vec3 snap_position() const { return vec::snap(this->raw_position); }

    virtual bool is_collidable() { return false; }

    virtual void render() const {
        if (this->model().has_value()) {
            DrawModel(this->model().value(), this->position,
                      this->size().x * 0.5f, ui::color::tan_brown);
        } else {
            DrawCube(position, this->size().x, this->size().y, this->size().z,
                     this->color);
        }
    }

    virtual void update_position(const vec3& p) { this->position = p; }

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

    virtual std::optional<Model> model() const { return {}; }
};

struct Bag : public Item {
    // TODO Are there likely to be other items that can hold items?
    std::shared_ptr<Item> held_item;

    // TODO we will eventually need a way to validate the kinds of items this
    // ItemContainer can hold

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Item>{});
        s.object(held_item);
    }

   public:
    Bag() {}
    Bag(vec3 p, Color c) : Item(p, c) {}
    Bag(vec2 p, Color c) : Item(p, c) {}

    bool empty() const { return held_item == nullptr; }

    virtual std::optional<Model> model() const override {
        if (empty()) {
            return ModelLibrary::get().get("empty_bag");
        }
        return ModelLibrary::get().get("bag");
    }
};
