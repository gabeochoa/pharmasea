
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
        addComponent<CustomHeldItemPosition>().init(
            CustomHeldItemPosition::Positioner::Default);
        get<HasWork>().init([](HasWork&, std::shared_ptr<Person>, float) {});
    }

    Furniture(vec3 pos, Color face_color_in)
        : Entity(pos, face_color_in, face_color_in) {
        add_static_components();
    }
    Furniture(vec2 pos, Color face_color_in, Color base_color_in)
        : Entity(pos, face_color_in, base_color_in) {
        add_static_components();
    }
    Furniture(vec2 pos, Color face_color_in)
        : Furniture(pos, face_color_in, face_color_in) {}

   public:
    // TODO this should be const
    virtual bool can_place_item_into(std::shared_ptr<Item> = nullptr) override {
        // TODO this should be a separate component
        return get<CanHoldItem>().empty();
    }

    virtual bool has_held_item() const {
        return get<CanHoldItem>().is_holding_item();
    }

    static Furniture* make_table(vec2 pos) {
        Furniture* table =
            new Furniture(pos, ui::color::brown, ui::color::brown);

        table->addComponent<CustomHeldItemPosition>().init(
            CustomHeldItemPosition::Positioner::Table);
        table->get<HasWork>().init(
            [](HasWork& hasWork, std::shared_ptr<Person>, float dt) {
                // TODO eventually we need it to decide whether it has work
                // based on the current held item
                const float amt = 0.5f;
                hasWork.pct_work_complete += amt * dt;
                if (hasWork.pct_work_complete >= 1.f)
                    hasWork.pct_work_complete = 0.f;
            });
        return table;
    }

    static Furniture* make_character_switcher(vec2 pos) {
        Furniture* character_switcher =
            new Furniture(pos, ui::color::brown, ui::color::brown);

        character_switcher->get<HasWork>().init(
            [](HasWork& hasWork, std::shared_ptr<Person> person, float dt) {
                const float amt = 2.f;
                hasWork.pct_work_complete += amt * dt;
                if (hasWork.pct_work_complete >= 1.f) {
                    hasWork.pct_work_complete = 0.f;
                    person->select_next_character_model();
                }
            });
        return character_switcher;
    }

    static Furniture* make_wall(vec2 pos, Color c) {
        Furniture* wall = new Furniture(pos, c, c);

        return wall;
        // enum Type {
        // FULL,
        // HALF,
        // QUARTER,
        // CORNER,
        // TEE,
        // DOUBLE_TEE,
        // };
        //
        // Type type = FULL;

        // TODO need to make sure we dont have this
        // its inherited from entity...
        // addComponent<CanHoldItem>();
        // virtual bool can_place_item_into(std::shared_ptr<Item>) override {
        // return false;
        // }

        // virtual void render_normal() const override {
        // TODO fix
        // switch (this->type) {
        // case Type::DOUBLE_TEE: {
        // DrawCubeCustom(this->raw_position,                        //
        // this->size().x / 2,                        //
        // this->size().y,                            //
        // this->size().z,                            //
        // FrontFaceDirectionMap.at(face_direction),  //
        // this->face_color, this->base_color);
        // DrawCubeCustom(this->raw_position,                        //
        // this->size().x,                            //
        // this->size().y,                            //
        // this->size().z / 2,                        //
        // FrontFaceDirectionMap.at(face_direction),  //
        // this->face_color, this->base_color);
        // } break;
        // case Type::FULL: {
        // DrawCubeCustom(this->raw_position,                        //
        // this->size().x,                            //
        // this->size().y,                            //
        // this->size().z,                            //
        // FrontFaceDirectionMap.at(face_direction),  //
        // this->face_color, this->base_color);
        // } break;
        // case Type::HALF: {
        // DrawCubeCustom(this->raw_position,                        //
        // this->size().x,                            //
        // this->size().y,                            //
        // this->size().z / 2,                        //
        // FrontFaceDirectionMap.at(face_direction),  //
        // this->face_color, this->base_color);
        // } break;
        // case Type::CORNER: {
        // DrawCubeCustom(this->raw_position,                        //
        // this->size().x / 2,                        //
        // this->size().y,                            //
        // this->size().z,                            //
        // FrontFaceDirectionMap.at(face_direction),  //
        // this->face_color, this->base_color);
        // DrawCubeCustom(this->raw_position,                        //
        // this->size().x,                            //
        // this->size().y,                            //
        // this->size().z / 2,                        //
        // FrontFaceDirectionMap.at(face_direction),  //
        // this->face_color, this->base_color);
        // } break;
        // case Type::TEE: {
        // DrawCubeCustom(this->raw_position,                        //
        // this->size().x / 2,                        //
        // this->size().y,                            //
        // this->size().z,                            //
        // FrontFaceDirectionMap.at(face_direction),  //
        // this->face_color, this->base_color);
        // DrawCubeCustom(this->raw_position,                        //
        // this->size().x,                            //
        // this->size().y,                            //
        // this->size().z / 2,                        //
        // FrontFaceDirectionMap.at(face_direction),  //
        // this->face_color, this->base_color);
        // } break;
        // case Type::QUARTER:
        // break;
        // }
        // }
    }
};
