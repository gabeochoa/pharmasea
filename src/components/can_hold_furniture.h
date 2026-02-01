

#pragma once

#include "../entity_ref.h"
#include "../vendor_include.h"
#include "base_component.h"

// Tag types for distinguishing held entity types
struct FurnitureHoldTag {};
struct HandTruckHoldTag {};

template <typename Tag>
struct CanHoldEntityBase : public BaseComponent {
    [[nodiscard]] bool empty() const {
        return held_entity.id == entity_id::INVALID;
    }
    [[nodiscard]] bool is_holding() const { return !empty(); }

    void update(EntityID entity_id, vec3 pickup_location) {
        held_entity.set_id(entity_id);
        pos = pickup_location;
    }

    [[nodiscard]] EntityID held_id() const { return held_entity.id; }
    [[nodiscard]] vec3 picked_up_at() const { return pos; }

   private:
    EntityRef held_entity{};
    vec3 pos;

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                         //
            static_cast<BaseComponent&>(self),  //
            self.held_entity,                   //
            self.pos                            //
        );
    }
};

// Type aliases for specific use cases
using CanHoldFurniture = CanHoldEntityBase<FurnitureHoldTag>;
using CanHoldHandTruck = CanHoldEntityBase<HandTruckHoldTag>;
