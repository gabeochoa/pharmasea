
#pragma once

#include "conveyer.h"

struct Grabber : public Conveyer {
   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Conveyer>{});
        // Only need to serialize things that are needed for render
    }

   public:
    Grabber() {}
    Grabber(vec2 pos) : Conveyer(pos) {}

    virtual void game_update(float dt) override {
        // Handle the normal item movement code
        Conveyer::game_update(dt);

        // We already have an item so
        if (this->held_item != nullptr) return;

        auto match = EntityHelper::getMatchingEntityInFront<Furniture>(
            vec::to2(this->position), 1.f,
            // Behind
            Entity::offsetFaceDirection(this->face_direction, 180),
            [this](std::shared_ptr<Furniture> furn) {
                return this->id != furn->id && furn->can_take_item_from();
            });

        // No furniture behind us
        if (!match) return;

        // Grab from the furniture match

        auto item = match->held_item;

        this->held_item = item;
        this->held_item->held_by = Item::HeldBy::FURNITURE;
        this->relative_item_pos = Conveyer::ITEM_START;

        match->held_item = nullptr;
    }
};
