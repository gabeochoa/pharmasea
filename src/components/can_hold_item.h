#pragma once

#include "../item.h"
#include "base_component.h"

struct CanHoldItem : public BaseComponent {
    virtual ~CanHoldItem() {}

    [[nodiscard]] bool empty() const { return held_item == nullptr; }
    [[nodiscard]] bool is_holding_item() const { return !empty(); }

    // TODO i feel like this is confusing to have with CanBeTakenFrom
    // this one means: "i have something you can take"
    // and the other one means "you are allowed to take things from me"

    // Whether or not this entity has something we can take from them
    [[nodiscard]] bool can_take_item_from() const { return !empty(); }

    template<typename T>
    [[nodiscard]] std::shared_ptr<T> asT() const {
        return dynamic_pointer_cast<T>(held_item);
    }

    void update(std::shared_ptr<Item> item,
                Item::HeldBy newHB = Item::HeldBy::NONE) {
        held_item = item;
        held_item->held_by = newHB;
    }

    // TODO this isnt const because we want to write to the item
    // we could make this const and then expose certain things that we want to
    // change separately like 'held_by'
    [[nodiscard]] std::shared_ptr<Item>& item() { return held_item; }

   private:
    std::shared_ptr<Item> held_item = nullptr;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.object(held_item);
    }
};
