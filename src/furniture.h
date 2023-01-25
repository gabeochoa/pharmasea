
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
        : Entity({pos.x, 0, pos.y}, face_color_in, base_color_in) {
        add_static_components();
    }

    Furniture(vec2 pos, Color face_color_in)
        : Furniture(pos, face_color_in, face_color_in) {}

   public:
    static Furniture* make_table(vec2 pos) {
        Furniture* table =
            new Furniture(pos, ui::color::brown, ui::color::brown);

        table->addComponent<CustomHeldItemPosition>().init(
            CustomHeldItemPosition::Positioner::Table);
        table->addComponent<HasWork>().init(
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
            new Furniture(pos, ui::color::brown, ui::color::brown);

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

    [[nodiscard]] static Furniture* make_conveyer(vec2 pos) {
        Furniture* conveyer =
            new Furniture(pos, ui::color::blue, ui::color::blue);
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
        Furniture* grabber =
            new Furniture(pos, ui::color::yellow, ui::color::yellow);
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
        Furniture* reg = new Furniture(pos, ui::color::grey, ui::color::grey);
        reg->addComponent<HasWaitingQueue>();

        reg->get<ModelRenderer>().update(ModelInfo{
            .model_name = "register",
            .size_scale = 10.f,
            .position_offset = vec3{0, -TILESIZE / 2.f, 0},
        });
        return reg;
    }
};

// TODO we can use concepts here to force this to be an Item
template<typename I>
struct ItemContainer : public Furniture {
   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Furniture>{});
    }

   public:
    ItemContainer() {}
    explicit ItemContainer(vec2 pos) : Furniture(pos, WHITE, WHITE) {
        this->addComponent<IsItemContainer<I>>();
    }

    virtual bool can_place_item_into(
        std::shared_ptr<Item> item = nullptr) override {
        return this->get<IsItemContainer<I>>().is_matching_item(item);
    }
};

struct BagBox : public ItemContainer<Bag> {
   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Furniture>{});
    }

   public:
    BagBox() {}
    explicit BagBox(vec2 pos) : ItemContainer<Bag>(pos) { update_model(); }

    void update_model() {
        // log_info("model index: {}", model_index);
        // TODO add a component for this
        const bool in_planning = GameState::get().is(game::State::Planning);
        get<ModelRenderer>().update(ModelInfo{
            .model_name = in_planning ? "box" : "open_box",
            .size_scale = 4.f,
            .position_offset = vec3{0, -TILESIZE / 2.f, 0},
        });
    }
};

struct MedicineCabinet : public ItemContainer<PillBottle> {
   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<ItemContainer>{});
    }

   public:
    MedicineCabinet() {}
    explicit MedicineCabinet(vec2 pos) : ItemContainer<PillBottle>(pos) {
        update_model();
    }

    void update_model() {
        // TODO add a component for this
        get<ModelRenderer>().update(ModelInfo{
            .model_name = "medicine_cabinet",
            .size_scale = 2.f,
            .position_offset = vec3{0, -TILESIZE / 2.f, 0},
        });
    }
};
