
#pragma once

#include "drawing_util.h"
#include "external_include.h"
//
#include "components/custom_item_position.h"
#include "components/has_work.h"
#include "components/is_rotatable.h"
#include "components/is_solid.h"
#include "entity.h"
#include "globals.h"
#include "person.h"

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
        addComponent<IsSolid>();
        addComponent<IsRotatable>();
        // addComponent<CustomHeldItemPosition>().init(
        // addComponent<CustomHeldItemPosition>().init(
        // [](Transform& transform) -> vec3 {
        // auto new_pos = transform.position;
        // new_pos.y += TILESIZE / 4;
        // return new_pos;
        // });
    }

    Furniture(vec2 pos, Color face_color_in)
        : Entity(pos, face_color_in, face_color_in) {
        add_static_components();
    }
    Furniture(vec3 pos, Color face_color_in)
        : Entity(pos, face_color_in, face_color_in) {
        add_static_components();
    }
    Furniture(vec2 pos, Color face_color_in, Color base_color_in)
        : Entity(pos, face_color_in, base_color_in) {
        add_static_components();
        get<HasWork>().init([](std::shared_ptr<Person>, float) {});
    }

   public:
    // TODO this should be const
    virtual bool add_to_navmesh() override { return true; }
    // TODO this should be const
    virtual bool can_be_picked_up() {
        return this->get<CanBeHeld>().is_not_held();
    }
    // TODO this should be const
    virtual bool can_place_item_into(std::shared_ptr<Item> = nullptr) override {
        // TODO this should be a separate component
        return get<CanHoldItem>().empty();
    }

    virtual bool has_held_item() const {
        return get<CanHoldItem>().is_holding_item();
    }
};
