

#pragma once

#include "../vendor_include.h"
#include "base_component.h"

#include "../entity_ref.h"

struct CanHoldHandTruck : public BaseComponent {
    [[nodiscard]] bool empty() const {
        return held_hand_truck.id == entity_id::INVALID;
    }
    [[nodiscard]] bool is_holding() const { return !empty(); }

    void update(EntityID hand_truck_id, vec3 pickup_location) {
        held_hand_truck.set_id(hand_truck_id);
        pos = pickup_location;
    }

    [[nodiscard]] EntityID hand_truck_id() const { return held_hand_truck.id; }
    [[nodiscard]] vec3 picked_up_at() const { return pos; }

   private:
    EntityRef held_hand_truck{};
    vec3 pos;

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                      //
            static_cast<BaseComponent&>(self), //
            self.held_hand_truck,            //
            self.pos                         //
        );
    }
};
