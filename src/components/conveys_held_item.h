

#pragma once

#include "base_component.h"

struct ConveysHeldItem : public BaseComponent {
    constexpr static float ITEM_START = -0.5f;
    constexpr static float ITEM_END = 0.5f;

    float SPEED = 0.5f;
    float relative_item_pos = ConveysHeldItem::ITEM_START;

    virtual ~ConveysHeldItem() {}

   private:
    friend class cereal::access;
    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseComponent>(this)
                // The reason we dont need to serialize this is that
                // we dont render the underlying value,
                // we use this value to move the entity and then serialize the
                // entity's position
        );
    }
};

CEREAL_REGISTER_TYPE(ConveysHeldItem);
