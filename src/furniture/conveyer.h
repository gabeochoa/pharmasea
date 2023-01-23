

#pragma once

#include "../external_include.h"
//
#include "../components/can_grab_from_other_furniture.h"
#include "../components/conveys_held_item.h"
#include "../components/custom_item_position.h"
#include "../entities.h"
#include "../furniture.h"

struct Conveyer : public Furniture {
    void add_static_components() {
        addComponent<CustomHeldItemPosition>().init(
            CustomHeldItemPosition::Positioner::Conveyer);
        addComponent<ConveysHeldItem>();
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Furniture>{});
        // Only need to serialize things that are needed for render
    }

   public:
    Conveyer() {}
    explicit Conveyer(vec2 pos)
        : Furniture(pos, ui::color::blue, ui::color::blue) {
        add_static_components();
        update_model();
    }
    Conveyer(vec2 pos, Color face_color, Color base_color)
        : Furniture(pos, face_color, base_color) {
        add_static_components();
        update_model();
    }

    void update_model() {
        // TODO add a component for this
        get<ModelRenderer>().update(ModelInfo{
            .model_name = "conveyer",
            .size_scale = 0.5f,
            .position_offset = vec3{0, 0, 0},
        });
    }

    // TODO fix
    bool can_take_item_from() const {
        return (get<CanHoldItem>().is_holding_item() && can_take_from);
    }

    virtual void in_round_update(float dt) override {
        Furniture::in_round_update(dt);
    }

    [[nodiscard]] static Conveyer* make_grabber(vec2 pos) {
        Conveyer* grabber =
            new Conveyer(pos, ui::color::yellow, ui::color::yellow);
        grabber->addComponent<ConveysHeldItem>();
        grabber->addComponent<CanGrabFromOtherFurniture>();
        return grabber;
    }
};
