#pragma once

#include "../item.h"
#include "base_component.h"

struct CanHoldItem : public BaseComponent {
    virtual ~CanHoldItem() {}

    [[nodiscard]] bool empty() const { return held_item == nullptr; }
    [[nodiscard]] bool is_holding_item() const { return !empty(); }
    // Whether or not this entity has something we can take from them
    [[nodiscard]] bool can_take_item_from() const { return !empty(); }

    template<typename T>
    [[nodiscard]] std::shared_ptr<T> asT() const {
        return dynamic_pointer_cast<T>(held_item);
    }

    void update(std::shared_ptr<Item> item) { held_item = item; }
    [[nodiscard]] std::shared_ptr<Item> item() const { return held_item; }

   private:
    std::shared_ptr<Item> held_item = nullptr;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.object(held_item);
    }
};
