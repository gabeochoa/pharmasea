

#pragma once

#include "base_component.h"

struct ConveysHeldItem : public BaseComponent {
    constexpr static float ITEM_START = -0.5f;
    constexpr static float ITEM_END = 0.5f;

    float SPEED = 0.5f;
    float relative_item_pos = ConveysHeldItem::ITEM_START;

   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        (void) self;
        return archive(                      //
            static_cast<BaseComponent&>(self)  //
        );

        // The reason we dont need to serialize this is that
        // we dont render the underlying value,
        // we use this value to move the entity and then serialize the entity's
        // position
    }
};
