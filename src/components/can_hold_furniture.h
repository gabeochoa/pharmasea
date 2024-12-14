

#pragma once

#include "../vendor_include.h"
#include "base_component.h"

using EntityID = int;

struct CanHoldFurniture : public BaseComponent {
    virtual ~CanHoldFurniture() {}

    [[nodiscard]] bool empty() const { return held_furniture_id == -1; }
    [[nodiscard]] bool is_holding_furniture() const { return !empty(); }

    void update(EntityID furniture_id, vec3 pickup_location) {
        held_furniture_id = furniture_id;
        pos = pickup_location;
    }

    [[nodiscard]] EntityID furniture_id() const { return held_furniture_id; }
    [[nodiscard]] vec3 picked_up_at() const { return pos; }

   private:
    int held_furniture_id = -1;
    vec3 pos;

    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseComponent>(this), held_furniture_id,
                pos);
    }
};

CEREAL_REGISTER_TYPE(CanHoldFurniture);
