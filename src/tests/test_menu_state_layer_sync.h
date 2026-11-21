#pragma once

#include "../engine/assert.h"
#include "../engine/statemanager.h"

namespace tests {

inline void test_menu_state_layer_sync() {
    using MenuStateT = menu::State;

    int calls = 0;
    MenuStateT seen = MenuStateT::Root;
    MenuState::get().register_on_change(
        [&calls, &seen](MenuStateT new_state, MenuStateT) {
            calls++;
            seen = new_state;
        });

    MenuState::get().reset();
    MenuState::get().transition_to(MenuStateT::Network);

    VALIDATE(calls == 1, "menu change should fire on_change exactly once");
    VALIDATE(seen == MenuStateT::Network,
             "on_change should receive the new menu state");

    MenuState::get().reset();
}

}  // namespace tests
