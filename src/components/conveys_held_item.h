

#pragma once

#include "base_component.h"

struct ConveysHeldItem : public BaseComponent {
    constexpr static float ITEM_START = -0.5f;
    constexpr static float ITEM_END = 0.5f;

    float SPEED = 0.5f;
    float relative_item_pos = ConveysHeldItem::ITEM_START;

    virtual ~ConveysHeldItem() {}

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        // The reason we dont need to serialize this is that
        // we dont render the underlying value,
        // we use this value to move the entity and then serialize the entity's
        // position
    }
};
