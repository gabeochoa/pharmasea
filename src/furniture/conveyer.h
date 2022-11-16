

#pragma once

#include "../external_include.h"
//
#include "../entities.h"
#include "../furniture.h"

struct Conveyer : public Furniture {
    float relative_item_pos = -0.5f;
    float SPEED = 0.5f;

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
        : Furniture(pos, ui::color::blue, ui::color::blue_green) {}

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

    virtual void update(float dt) override {
        std::cout << this->id << " " << this->is_held << " "
                  << this->is_collidable() << std::endl;
        this->is_highlighted = false;
        Furniture::update(dt);
        // we are not holding anything
        if (this->held_item == nullptr) {
            return;
        }

        // if the item is less than halfway, just keep moving it along
        if (relative_item_pos < 0.f) {
            relative_item_pos += SPEED * dt;
            return;
        }

        auto match = EntityHelper::getMatchingEntityInFront<Furniture>(
            vec::to2(this->position), 1.f, this->face_direction,
            [](std::shared_ptr<Furniture> furn) {
                return furn->can_place_item_into();
            });
        // no match means we cant continue, stay in the middle
        if (!match) {
            relative_item_pos = 0.f;
            return;
        }

        // we got something that will take from us,
        // but only once we get close enough

        // so keep moving forward
        if (relative_item_pos < 0.5f) {
            relative_item_pos += SPEED * dt;
            return;
        }

        // we reached the end, pass ownership
        match->held_item = this->held_item;
        match->held_item->held_by = Item::HeldBy::FURNITURE;
        this->held_item = nullptr;
        this->relative_item_pos = -0.5f;

        // TODO if we are pushing onto a conveyer, we need to make sure
        // we are keeping track of the orientations
        //
        //  --> --> in this case we want to place at 0.f
        //
        //          ^
        //    -->-> |     in this we want to place at 0.f instead of -0.5
    }
};
