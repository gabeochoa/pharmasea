#pragma once

#include "../components/is_item_container.h"
#include "../engine/assert.h"

namespace tests {

inline void test_item_container() {
    using ContainerState = IsItemContainer::ContainerState;

    IsItemContainer container;
    VALIDATE(container.container_state() == ContainerState::Empty,
             "new container should be empty");
    VALIDATE(!container.hit_max(),
             "unlimited container should never hit max");

    container.increment();
    VALIDATE(container.container_state() == ContainerState::Stocked,
             "container with generations should be stocked");

    container.set_max_generations(1);
    VALIDATE(container.container_state() == ContainerState::Exhausted,
             "container at generation cap should be exhausted");
    VALIDATE(container.is_container_state(ContainerState::Exhausted),
             "is_container_state should agree with container_state()");
}

}  // namespace tests
