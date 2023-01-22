
#pragma once

#include "drawing_util.h"
#include "external_include.h"
//
#include "components/custom_item_position.h"
#include "components/has_work.h"
#include "entity.h"
#include "globals.h"

struct Furniture : public Entity {
   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Entity>{});
        // Only need to serialize things that are needed for render
    }

   protected:
    Furniture() : Entity() {}

    void add_static_components() {
        addComponent<HasWork>();
        // addComponent<CustomHeldItemPosition>().init(
        // addComponent<CustomHeldItemPosition>().init(
        // [](Transform& transform) -> vec3 {
        // auto new_pos = transform.position;
        // new_pos.y += TILESIZE / 4;
        // return new_pos;
        // });
        get<HasWork>().init([](std::shared_ptr<Person>, float) {});
    }

   public:
    Furniture(vec3 pos, Color) : Entity(pos) { add_static_components(); }
    Furniture(vec2 pos, Color) : Entity(pos) { add_static_components(); }

    // TODO this should be const
    virtual bool add_to_navmesh() override { return true; }
    virtual bool can_rotate() const { return true; }
    // TODO this should be const
    virtual bool can_be_picked_up() { return !this->is_held; }
    // TODO this should be const
    virtual bool can_place_item_into(std::shared_ptr<Item> = nullptr) override {
        // TODO this should be a separate component
        return get<CanHoldItem>().empty();
    }

    virtual bool has_held_item() const {
        return get<CanHoldItem>().is_holding_item();
    }
};
