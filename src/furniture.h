
#pragma once

#include "external_include.h"
//
#include "entity.h"
#include "globals.h"

struct Furniture : public Entity {
    struct ModelInfo {
        Model model;
        float size_scale;
        vec3 position_offset;
    };

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

    virtual std::optional<ModelInfo> model() const { return {}; }

    virtual void render_highlighted() const override {
        if (model().has_value()) {
            Color base = ui::color::getHighlighted(this->base_color);

            float rotation_angle =
                180.f +
                static_cast<int>(FrontFaceDirectionMap.at(face_direction));

            ModelInfo model_info = model().value();

            DrawModelEx(model_info.model,
                        {
                            this->position.x + model_info.position_offset.x,
                            this->position.y + model_info.position_offset.y,
                            this->position.z + model_info.position_offset.z,
                        },
                        vec3{0.f, 1.f, 0.f}, rotation_angle,
                        this->size() * model_info.size_scale, base);

        } else {
            Entity::render_highlighted();
        }
    }

    virtual void render_normal() const override {
        if (model().has_value()) {
            float rotation_angle =
                180.f +
                static_cast<int>(FrontFaceDirectionMap.at(face_direction));

            ModelInfo model_info = model().value();

            DrawModelEx(model_info.model,
                        {
                            this->position.x + model_info.position_offset.x,
                            this->position.y + model_info.position_offset.y,
                            this->position.z + model_info.position_offset.z,
                        },
                        vec3{0.f, 1.f, 0.f}, rotation_angle,
                        this->size() * model_info.size_scale, this->base_color);

        } else {
            Entity::render_normal();
        }
    }

    virtual bool add_to_navmesh() override { return true; }
    virtual bool can_rotate() const { return true; }
    virtual bool can_be_picked_up() { return !this->is_held; }
    virtual bool can_place_item_into(std::shared_ptr<Item> = nullptr) override {
        return this->held_item == nullptr;
    }
};
