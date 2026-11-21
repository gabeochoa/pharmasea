#pragma once

#include "../components/can_hold_item.h"
#include "../components/is_item.h"
#include "../engine/assert.h"
#include "../entities/entity_helper.h"

namespace tests {

inline void test_held_item_state() {
    using HeldState = CanHoldItem::HeldState;

    CanHoldItem holder(EntityType::Player);
    VALIDATE(holder.held_state() == HeldState::Empty,
             "new holder should be empty");

    Entity& item = EntityHelper::createEntity();
    item.addComponent<IsItem>();
    holder.update(item, 77);

    VALIDATE(holder.held_state() == HeldState::Holding,
             "holder with an item should be holding");
    VALIDATE(holder.is_held_state(HeldState::Holding),
             "is_held_state should agree with held_state()");
    VALIDATE(item.get<IsItem>().holder() == 77,
             "item back-pointer should record the holder id");
    VALIDATE(item.get<IsItem>().is_held_by(EntityType::Player),
             "item back-pointer should record the holder type");

    holder.update(nullptr, 77);
    VALIDATE(holder.held_state() == HeldState::Empty,
             "cleared holder should be empty");
}

}  // namespace tests
