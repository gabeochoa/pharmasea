

#pragma once

#include "../external_include.h"
//
#include "../components/custom_item_position.h"
#include "../entities.h"
#include "../furniture.h"

struct Conveyer : public Furniture {
    constexpr static float ITEM_START = -0.5f;
    constexpr static float ITEM_END = 0.5f;

    float relative_item_pos = Conveyer::ITEM_START;
    float SPEED = 0.5f;
    bool can_take_from = false;

    void add_static_components() {
        addComponent<CustomHeldItemPosition>().init(
            CustomHeldItemPosition::Positioner::Conveyer);
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Furniture>{});
        // Only need to serialize things that are needed for render
        s.value4b(relative_item_pos);
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
        Transform& transform = this->get<Transform>();

        can_take_from = false;

        // we are not holding anything
        if (get<CanHoldItem>().empty()) {
            return;
        }

        // if the item is less than halfway, just keep moving it along
        if (relative_item_pos <= 0.f) {
            relative_item_pos += SPEED * dt;
            return;
        }

        auto match = EntityHelper::getMatchingEntityInFront<Furniture>(
            transform.as2(), 1.f, transform.face_direction,
            [this](std::shared_ptr<Furniture> furn) {
                return this->id != furn->id &&
                       // TODO need to merge this into the system manager one
                       // but cant yet
                       furn->get<CanHoldItem>().empty();
            });
        // no match means we cant continue, stay in the middle
        if (!match) {
            relative_item_pos = 0.f;
            can_take_from = true;
            return;
        }

        // we got something that will take from us,
        // but only once we get close enough

        // so keep moving forward
        if (relative_item_pos <= Conveyer::ITEM_END) {
            relative_item_pos += SPEED * dt;
            return;
        }

        can_take_from = true;
        // we reached the end, pass ownership

        CanHoldItem& matchCHI = match->get<CanHoldItem>();
        CanHoldItem& ourCHI = this->get<CanHoldItem>();

        matchCHI.item() = ourCHI.item();
        matchCHI.item()->held_by = Item::HeldBy::FURNITURE;
        ourCHI.item() = nullptr;
        this->relative_item_pos = Conveyer::ITEM_START;

        // TODO if we are pushing onto a conveyer, we need to make sure
        // we are keeping track of the orientations
        //
        //  --> --> in this case we want to place at 0.f
        //
        //          ^
        //    -->-> |     in this we want to place at 0.f instead of -0.5
    }
};
