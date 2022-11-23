

#pragma once

#include "../external_include.h"
//
#include "../entities.h"
#include "../furniture.h"

struct Conveyer : public Furniture {
    constexpr static float ITEM_START = -0.5f;
    constexpr static float ITEM_END = 0.5f;

    float relative_item_pos = Conveyer::ITEM_START;
    float SPEED = 0.5f;
    bool can_take_from = false;

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
    Conveyer(vec2 pos)
        : Furniture(pos, ui::color::light_grey, ui::color::light_grey) {}

    virtual std::optional<ModelInfo> model() const override {
        return ModelInfo{
            .model = ModelLibrary::get().get("conveyer"),
            .size_scale = 0.5f,
            .position_offset = vec3{0, 0, 0},
        };
    }

    virtual bool can_take_item_from() const override {
        return (this->held_item != nullptr && can_take_from);
    }

    virtual void update_held_item_position() override {
        if (held_item != nullptr) {
            auto new_pos = this->position;
            if (this->face_direction & FrontFaceDirection::FORWARD) {
                new_pos.z += TILESIZE * relative_item_pos;
            }
            if (this->face_direction & FrontFaceDirection::RIGHT) {
                new_pos.x += TILESIZE * relative_item_pos;
            }
            if (this->face_direction & FrontFaceDirection::BACK) {
                new_pos.z -= TILESIZE * relative_item_pos;
            }
            if (this->face_direction & FrontFaceDirection::LEFT) {
                new_pos.x -= TILESIZE * relative_item_pos;
            }

            held_item->update_position(new_pos);
        }
    }

    virtual void game_update(float dt) override {
        Furniture::game_update(dt);

        can_take_from = false;

        // we are not holding anything
        if (this->held_item == nullptr) {
            return;
        }

        // if the item is less than halfway, just keep moving it along
        if (relative_item_pos <= 0.f) {
            relative_item_pos += SPEED * dt;
            return;
        }

        auto match = EntityHelper::getMatchingEntityInFront<Furniture>(
            vec::to2(this->position), 1.f, this->face_direction,
            [this](std::shared_ptr<Furniture> furn) {
                return this->id != furn->id && furn->can_place_item_into();
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
        match->held_item = this->held_item;
        match->held_item->held_by = Item::HeldBy::FURNITURE;
        this->held_item = nullptr;
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
