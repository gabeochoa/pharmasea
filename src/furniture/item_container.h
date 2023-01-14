
#pragma once

#include "../external_include.h"
//
#include "../entity.h"
#include "../globals.h"
//
#include "../aiperson.h"
#include "../furniture.h"
#include "../menu.h"

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
    explicit ItemContainer(vec2 pos) : Furniture(pos, WHITE, WHITE) {}

    virtual bool can_place_item_into(
        std::shared_ptr<Item> item = nullptr) override {
        auto i = dynamic_pointer_cast<I>(item);
        if (!i) return false;
        if (!i->empty()) return false;
        return true;
    }

    virtual void game_update(float dt) override {
        Furniture::game_update(dt);
        if (this->held_item == nullptr) {
            this->held_item.reset(
                new I(this->position, (Color){255, 15, 240, 255}));
            ItemHelper::addItem(this->held_item);
        }
    }
};
