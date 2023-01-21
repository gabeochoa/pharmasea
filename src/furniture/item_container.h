
#pragma once

#include "../external_include.h"
//
#include "../entity.h"
#include "../globals.h"
//
#include "../aiperson.h"
#include "../furniture.h"

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
    explicit ItemContainer(vec2 pos) : Furniture(pos, WHITE, WHITE) {}

    virtual bool can_place_item_into(
        std::shared_ptr<Item> item = nullptr) override {
        auto i = dynamic_pointer_cast<I>(item);
        if (!i) return false;
        if (!i->empty()) return false;
        return true;
    }

    virtual void in_round_update(float dt) override {
        Furniture::in_round_update(dt);
        if (this->held_item() == nullptr) {
            this->held_item().reset(
                // TODO what is this color and what is it for
                new I(this->get<Transform>().position,
                      Color({255, 15, 240, 255})));
            ItemHelper::addItem(this->held_item());
        }
    }
};
