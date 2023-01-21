#pragma once

//
#include "../item.h"
#include "base_component.h"

struct CanHoldItem : public BaseComponent {
    std::shared_ptr<Item> held_item = nullptr;

    virtual ~CanHoldItem() {}

    [[nodiscard]] bool is_holding_item() const { return held_item != nullptr; }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.object(held_item);
    }
};
