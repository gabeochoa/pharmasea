

#pragma once

#include "../entity_ref.h"
#include "../vendor_include.h"
#include "base_component.h"

struct CanHoldFurniture : public BaseComponent {
    [[nodiscard]] bool empty() const {
        return held_furniture.id == entity_id::INVALID;
    }
    [[nodiscard]] bool is_holding_furniture() const { return !empty(); }

    void update(EntityID furniture_id, vec3 pickup_location) {
        held_furniture.set_id(furniture_id);
        pos = pickup_location;
    }

    [[nodiscard]] EntityID furniture_id() const { return held_furniture.id; }
    [[nodiscard]] vec3 picked_up_at() const { return pos; }

   private:
    EntityRef held_furniture{};
    vec3 pos;

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                         //
            static_cast<BaseComponent&>(self),  //
            self.held_furniture,                //
            self.pos                            //
        );
    }
};
