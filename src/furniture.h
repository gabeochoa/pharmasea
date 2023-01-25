
#pragma once

#include "aiperson.h"
#include "components/can_grab_from_other_furniture.h"
#include "components/conveys_held_item.h"
#include "components/custom_item_position.h"
#include "components/has_waiting_queue.h"
#include "components/has_work.h"
#include "components/is_item_container.h"
#include "components/is_rotatable.h"
#include "components/is_solid.h"
#include "components/shows_progress_bar.h"
#include "drawing_util.h"
#include "engine/assert.h"
#include "entity.h"
#include "external_include.h"
#include "furniture.h"
#include "globals.h"
#include "person.h"
#include "statemanager.h"

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

    Furniture(vec3 pos, Color face_color_in)
        : Entity(pos, face_color_in, face_color_in) {}
    Furniture(vec2 pos, Color face_color_in, Color base_color_in)
        : Entity({pos.x, 0, pos.y}, face_color_in, base_color_in) {}

    Furniture(vec2 pos, Color face_color_in)
        : Furniture(pos, face_color_in, face_color_in) {}

   public:
    static Furniture* make_furniture(vec2 pos, Color face, Color base) {
        Furniture* furniture = new Furniture(pos, face, base);

        furniture->addComponent<IsSolid>();
        furniture->addComponent<IsRotatable>();

        furniture->addComponent<HasWork>().init(
            [](HasWork&, std::shared_ptr<Person>, float) {});
        furniture->addComponent<CustomHeldItemPosition>().init(
            CustomHeldItemPosition::Positioner::Default);

        return furniture;
    }

    static Furniture* make_table(vec2 pos) {
        Furniture* table =
            Furniture::make_furniture(pos, ui::color::brown, ui::color::brown);

        table->get<CustomHeldItemPosition>().init(
            CustomHeldItemPosition::Positioner::Table);

        table->get<HasWork>().init(
            [](HasWork& hasWork, std::shared_ptr<Person>, float dt) {
                // TODO eventually we need it to decide whether it has work
                // based on the current held item
                const float amt = 0.5f;
                hasWork.increase_pct(amt * dt);
                if (hasWork.is_work_complete()) hasWork.reset_pct();
            });
        table->addComponent<ShowsProgressBar>();
        return table;
    }

    static Furniture* make_character_switcher(vec2 pos) {
        Furniture* character_switcher =
            Furniture::make_furniture(pos, ui::color::brown, ui::color::brown);

        character_switcher->get<HasWork>().init(
            [](HasWork& hasWork, std::shared_ptr<Person> person, float dt) {
                if (!person->has<UsesCharacterModel>()) return;
                UsesCharacterModel& usesCharacterModel =
                    person->get<UsesCharacterModel>();

                const float amt = 2.f;
                hasWork.increase_pct(amt * dt);
                if (hasWork.is_work_complete()) {
                    hasWork.reset_pct();
                    usesCharacterModel.increment();
                }
            });
        character_switcher->addComponent<ShowsProgressBar>();
        return character_switcher;
    }

    static Furniture* make_wall(vec2 pos, Color c) {
        Furniture* wall =
            Furniture::make_furniture(pos, ui::color::brown, ui::color::brown);

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

    [[nodiscard]] static Furniture* make_conveyer(vec2 pos) {
        Furniture* conveyer =
            Furniture::make_furniture(pos, ui::color::blue, ui::color::blue);
        // TODO fix
        // bool can_take_item_from() const {
        // return (get<CanHoldItem>().is_holding_item() && can_take_from);
        // }

        conveyer->addComponent<CustomHeldItemPosition>().init(
            CustomHeldItemPosition::Positioner::Conveyer);
        conveyer->addComponent<ConveysHeldItem>();

        // TODO add a component for this
        conveyer->get<ModelRenderer>().update(ModelInfo{
            .model_name = "conveyer",
            .size_scale = 0.5f,
            .position_offset = vec3{0, 0, 0},
        });

        return conveyer;
    }

    [[nodiscard]] static Furniture* make_grabber(vec2 pos) {
        Furniture* grabber = Furniture::make_furniture(pos, ui::color::yellow,
                                                       ui::color::yellow);
        // TODO fix
        // bool can_take_item_from() const {
        // return (get<CanHoldItem>().is_holding_item() && can_take_from);
        // }
        grabber->get<ModelRenderer>().update(ModelInfo{
            .model_name = "conveyer",
            .size_scale = 0.5f,
            .position_offset = vec3{0, 0, 0},
        });
        grabber->addComponent<ConveysHeldItem>();
        grabber->addComponent<CanGrabFromOtherFurniture>();
        return grabber;
    }

    [[nodiscard]] static Furniture* make_register(vec2 pos) {
        Furniture* reg =
            Furniture::make_furniture(pos, ui::color::grey, ui::color::grey);
        reg->addComponent<HasWaitingQueue>();

        reg->get<ModelRenderer>().update(ModelInfo{
            .model_name = "register",
            .size_scale = 10.f,
            .position_offset = vec3{0, -TILESIZE / 2.f, 0},
        });
        return reg;
    }

    template<typename I>
    [[nodiscard]] static Furniture* make_itemcontainer(vec2 pos) {
        Furniture* container =
            Furniture::make_furniture(pos, ui::color::white, ui::color::white);
        container->addComponent<IsItemContainer<I>>();
        return container;
        // virtual bool can_place_item_into(
        // std::shared_ptr<Item> item = nullptr) override {
        // return this->get<IsItemContainer<I>>().is_matching_item(item);
        // }
    }

    [[nodiscard]] static Furniture* make_bagbox(vec2 pos) {
        Furniture* container = Furniture::make_itemcontainer<Bag>(pos);

        const bool in_planning = GameState::get().is(game::State::Planning);
        container->get<ModelRenderer>().update(ModelInfo{
            .model_name = in_planning ? "box" : "open_box",
            .size_scale = 4.f,
            .position_offset = vec3{0, -TILESIZE / 2.f, 0},
        });
        return container;
    }

    [[nodiscard]] static Furniture* make_medicine_cabinet(vec2 pos) {
        Furniture* container = Furniture::make_itemcontainer<PillBottle>(pos);
        container->get<ModelRenderer>().update(ModelInfo{
            .model_name = "medicine_cabinet",
            .size_scale = 2.f,
            .position_offset = vec3{0, -TILESIZE / 2.f, 0},
        });
        return container;
    }
};
