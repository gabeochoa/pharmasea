
#pragma once

#include "external_include.h"
//
#include "entity.h"
#include "globals.h"

struct Furniture : public Entity {
   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Entity>{});
    }

   public:
    Furniture() : Entity() {}
    Furniture(vec2 pos, Color face_color_in)
        : Entity(pos, face_color_in, face_color_in) {}
    Furniture(vec3 pos, Color face_color_in)
        : Entity(pos, face_color_in, face_color_in) {}
    Furniture(vec2 pos, Color face_color_in, Color base_color_in)
        : Entity(pos, face_color_in, base_color_in) {}

    virtual void update_held_item_position() override {
        if (held_item != nullptr) {
            auto new_pos = this->position;
            new_pos.y += TILESIZE / 4;
            held_item->update_position(new_pos);
        }
    }

    virtual std::optional<Model> model() const { return {}; }

    virtual void render() const override {
        if (model().has_value()) {
            Color base = this->is_highlighted
                             ? ui::color::getHighlighted(this->base_color)
                             : this->base_color;
            float rotation_angle =
                180.f +
                static_cast<int>(FrontFaceDirectionMap.at(face_direction));

            DrawModelEx(model().value(),
                        {
                            this->position.x,
                            this->position.y - TILESIZE / 2,
                            this->position.z,
                        },
                        vec3{0.f, 1.f, 0.f}, rotation_angle,
                        this->size() * 10.f, base);

        } else {
            Entity::render();
        }
    }

    virtual bool add_to_navmesh() override { return true; }
    virtual bool can_rotate() { return true; }
    virtual bool can_be_picked_up() { return !this->is_held; }
    virtual bool can_place_item_into() override {
        return this->held_item == nullptr;
    }
};
