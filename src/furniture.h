
#pragma once

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
#include "engine/statemanager.h"
#include "entity.h"
#include "external_include.h"
#include "furniture.h"
#include "globals.h"
#include "person.h"

typedef Entity Furniture;

namespace entities {
static Entity* make_furniture(vec2 pos, Color face, Color base) {
    Entity* furniture = new Entity({pos.x, 0, pos.y}, face, base);

    furniture->addComponent<IsSolid>();
    furniture->addComponent<IsRotatable>();

    furniture->addComponent<CustomHeldItemPosition>().init(
        CustomHeldItemPosition::Positioner::Default);

    return furniture;
}

static Entity* make_table(vec2 pos) {
    Entity* table =
        entities::make_furniture(pos, ui::color::brown, ui::color::brown);

    table->get<CustomHeldItemPosition>().init(
        CustomHeldItemPosition::Positioner::Table);

    table->addComponent<HasWork>().init(
        [](HasWork& hasWork, std::shared_ptr<Entity>, float dt) {
            // TODO eventually we need it to decide whether it has work
            // based on the current held item
            const float amt = 0.5f;
            hasWork.increase_pct(amt * dt);
            if (hasWork.is_work_complete()) hasWork.reset_pct();
        });
    table->addComponent<ShowsProgressBar>();
    return table;
}

static Entity* make_character_switcher(vec2 pos) {
    Entity* character_switcher =
        entities::make_furniture(pos, ui::color::brown, ui::color::brown);

    character_switcher->addComponent<HasWork>().init(
        [](HasWork& hasWork, std::shared_ptr<Entity> person, float dt) {
            if (person->is_missing<UsesCharacterModel>()) return;
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

static Entity* make_wall(vec2 pos, Color c) {
    Entity* wall =
        entities::make_furniture(pos, ui::color::brown, ui::color::brown);

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

[[nodiscard]] static Entity* make_conveyer(vec2 pos) {
    Entity* conveyer =
        entities::make_furniture(pos, ui::color::blue, ui::color::blue);
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

[[nodiscard]] static Entity* make_grabber(vec2 pos) {
    Entity* grabber =
        entities::make_furniture(pos, ui::color::yellow, ui::color::yellow);
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

[[nodiscard]] static Entity* make_register(vec2 pos) {
    Entity* reg =
        entities::make_furniture(pos, ui::color::grey, ui::color::grey);
    reg->addComponent<HasWaitingQueue>();

    reg->get<ModelRenderer>().update(ModelInfo{
        .model_name = "register",
        .size_scale = 10.f,
        .position_offset = vec3{0, -TILESIZE / 2.f, 0},
    });
    return reg;
}

template<typename I>
[[nodiscard]] static Entity* make_itemcontainer(vec2 pos) {
    Entity* container =
        entities::make_furniture(pos, ui::color::white, ui::color::white);
    container->addComponent<IsItemContainer<I>>();
    return container;
    // virtual bool can_place_item_into(
    // std::shared_ptr<Item> item = nullptr) override {
    // return this->get<IsItemContainer<I>>().is_matching_item(item);
    // }
}

[[nodiscard]] static Entity* make_bagbox(vec2 pos) {
    Entity* container = entities::make_itemcontainer<Bag>(pos);

    const bool in_planning = GameState::get().is(game::State::Planning);
    container->get<ModelRenderer>().update(ModelInfo{
        .model_name = in_planning ? "box" : "open_box",
        .size_scale = 4.f,
        .position_offset = vec3{0, -TILESIZE / 2.f, 0},
    });
    return container;
}

[[nodiscard]] static Entity* make_medicine_cabinet(vec2 pos) {
    Entity* container = entities::make_itemcontainer<PillBottle>(pos);
    container->get<ModelRenderer>().update(ModelInfo{
        .model_name = "medicine_cabinet",
        .size_scale = 2.f,
        .position_offset = vec3{0, -TILESIZE / 2.f, 0},
    });
    return container;
}
}  // namespace entities
