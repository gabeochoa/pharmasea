#pragma once

#include "external_include.h"
//
#include "engine.h"
#include "engine/model_library.h"
#include "engine/random.h"
#include "globals.h"
#include "raylib.h"
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

    // TODO Are there likely to be other items that can hold items?
    std::shared_ptr<Item> held_item;

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
        s.object(held_item);
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

    virtual vec3 model_scale() const { return {0.5f, 0.5f, 0.5f}; }

    virtual void render(bool render_held_item = true) const {
        // Dont render when held by another item
        // but do render if we are being rendered by our parent
        if (held_by == HeldBy::ITEM && !render_held_item) {
            return;
        }

        if (this->model().has_value()) {
            const auto sz = this->size();
            const auto m_sc = this->model_scale();
            const vec3 scale = {
                sz.x * m_sc.x,
                sz.y * m_sc.y,
                sz.z * m_sc.z,
            };
            raylib::DrawModelEx(this->model().value(), this->position,
                                {0, 1.f, 0}, 0 /* rotation angle */, scale,
                                ui::color::white);
        } else {
            raylib::DrawCube(position, this->size().x, this->size().y,
                             this->size().z, this->color);
        }

        if (this->held_item != nullptr && render_held_item) {
            this->held_item->render(false);
        }
    }

    virtual void update_position(const vec3& p) {
        this->position = p;
        if (this->held_item != nullptr) {
            this->held_item->update_position(p);
        }
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
        return raylib::CheckCollisionBoxes(this->bounds(), b);
    }

    virtual std::optional<raylib::Model> model() const { return {}; }

    // TODO maybe use tl::expected for this?
    virtual bool eat(std::shared_ptr<Item> item) {
        if (!can_eat(item)) return false;

        this->held_item = item;
        item->held_by = Item::HeldBy::ITEM;

        return true;
    }

    virtual bool can_eat(std::shared_ptr<Item> item) {
        if (!has_holding_ability()) {
            log_info("cant eat because we cant hold things");
            return false;
        }
        // Note; this is done here to avoid -Wpotentially-evaluated-expression
        // warning
        auto& underlying = *item;
        if (typeid(underlying) == typeid(*this)) {
            log_info("cant put an item into an item of the same type");
            return false;
        }
        if (!empty()) {
            log_info("cant put an item into because we are full");
            return false;
        }
        // TODO we will eventually need a way to validate the kinds of items
        // this ItemContainer can hold
        return true;
    }

    virtual bool has_holding_ability() const { return false; }

    virtual bool empty() const {
        return has_holding_ability() && held_item == nullptr;
    }
};

struct Pill : public Item {
    enum struct PillType {
        Red,
        RedLong,
        Blue,
        BlueLong,
    } type;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Item>{});
        s.value4b(type);
    }

   public:
    Pill() { init(); }
    Pill(vec3 p, Color c) : Item(p, c) { init(); }
    Pill(vec2 p, Color c) : Item(p, c) { init(); }

    void init() {
        constexpr std::size_t num_pills = magic_enum::enum_count<PillType>();
        int index = randIn(0, num_pills - 1);
        type = magic_enum::enum_value<PillType>(index);
    }

    virtual vec3 model_scale() const override { return {3.f, 3.0f, 12.f}; }

    virtual std::optional<raylib::Model> model() const override {
        switch (type) {
            case PillType::Red:
                return ModelLibrary::get().get("pill_red");
            case PillType::RedLong:
                return ModelLibrary::get().get("pill_redlong");
            case PillType::Blue:
                return ModelLibrary::get().get("pill_blue");
            case PillType::BlueLong:
                return ModelLibrary::get().get("pill_bluelong");
        }
        log_warn("Failed to get matching model for pill type: {}",
                 magic_enum::enum_name(type));
        return {};
    }
};

struct PillBottle : public Item {
   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Item>{});
    }

   public:
    PillBottle() {}
    PillBottle(vec3 p, Color c) : Item(p, c) {}
    PillBottle(vec2 p, Color c) : Item(p, c) {}

    virtual vec3 model_scale() const override { return {3.f, 3.0f, 3.f}; }

    virtual std::optional<raylib::Model> model() const override {
        // TODO handle empty vs full
        return ModelLibrary::get().get("pill_bottle");
    }
};

struct Bag : public Item {
   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Item>{});
    }

   public:
    Bag() {}
    Bag(vec3 p, Color c) : Item(p, c) {}
    Bag(vec2 p, Color c) : Item(p, c) {}

    virtual bool has_holding_ability() const override { return true; }

    virtual std::optional<raylib::Model> model() const override {
        if (empty()) {
            return ModelLibrary::get().get("empty_bag");
        }
        return ModelLibrary::get().get("bag");
    }
};
