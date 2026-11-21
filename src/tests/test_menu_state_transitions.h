#pragma once

#include "../engine/assert.h"
#include "../engine/statemanager.h"

namespace tests {

inline void test_menu_state_transitions() {
    using MenuStateT = menu::State;

    VALIDATE(MenuState::is_legal_transition(MenuStateT::Root, MenuStateT::Network),
             "Root should allow Network");
    VALIDATE(MenuState::is_legal_transition(MenuStateT::Root, MenuStateT::Settings),
             "Root should allow Settings");
    VALIDATE(MenuState::is_legal_transition(MenuStateT::Network, MenuStateT::Game),
             "Network should allow Game");
    VALIDATE(MenuState::is_legal_transition(MenuStateT::Game, MenuStateT::Settings),
             "Game should allow Settings from pause");
    VALIDATE(!MenuState::is_legal_transition(MenuStateT::Root, MenuStateT::Game),
             "Root should not jump straight to Game");
    VALIDATE(!MenuState::is_legal_transition(MenuStateT::Settings, MenuStateT::Game),
             "Settings should exit via back, not a direct Game set");

    VALIDATE(MenuState::is_host_sync_legal(MenuStateT::Network, MenuStateT::Game),
             "host sync should allow Network to Game");
    VALIDATE(!MenuState::is_host_sync_legal(MenuStateT::Settings, MenuStateT::Game),
             "host sync must not yank a client out of Settings");
    VALIDATE(!MenuState::is_host_sync_legal(MenuStateT::Root, MenuStateT::Game),
             "host sync should only fire from Network");

    MenuState::get().reset();
    VALIDATE(MenuState::get().transition_to(MenuStateT::Network),
             "transition_to Network from Root should be legal");
    VALIDATE(MenuState::get().is(MenuStateT::Network),
             "transition_to should apply the new state");
    MenuState::get().reset();
}

}  // namespace tests
