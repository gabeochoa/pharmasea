
#pragma once

#include "../item.h"
#include "base_component.h"

template<typename I>
struct IsItemContainer : public BaseComponent {
    virtual ~IsItemContainer() {}

    [[nodiscard]] virtual bool is_matching_item(
        std::shared_ptr<Item> item = nullptr) {
        auto i = dynamic_pointer_cast<I>(item);
        if (!i) return false;
        if (!i->empty()) return false;
        return true;
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
    }
};
