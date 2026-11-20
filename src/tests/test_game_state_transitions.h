#pragma once

#include "../engine/assert.h"
#include "../engine/statemanager.h"

namespace tests {

inline void test_game_state_transitions() {
    using GameStateT = game::State;

    VALIDATE(GameState::is_legal_transition(GameStateT::InMenu, GameStateT::Lobby),
             "InMenu should allow Lobby");
    VALIDATE(GameState::is_legal_transition(GameStateT::Lobby, GameStateT::InGame),
             "Lobby should allow InGame");
    VALIDATE(GameState::is_legal_transition(GameStateT::Lobby, GameStateT::ModelTest),
             "Lobby should allow ModelTest");
    VALIDATE(GameState::is_legal_transition(GameStateT::Lobby,
                                            GameStateT::LoadSaveRoom),
             "Lobby should allow LoadSaveRoom");
    VALIDATE(GameState::is_legal_transition(GameStateT::InGame, GameStateT::Paused),
             "InGame should allow Paused");
    VALIDATE(!GameState::is_legal_transition(GameStateT::InMenu, GameStateT::InGame),
             "InMenu should not jump straight to InGame");
    VALIDATE(!GameState::is_legal_transition(GameStateT::InMenu, GameStateT::ModelTest),
             "InMenu should not jump straight to ModelTest");
    VALIDATE(!GameState::is_legal_transition(GameStateT::Paused, GameStateT::InGame),
             "Paused should resume via go_back, not transition_to");

    GameState::get().reset();
    VALIDATE(GameState::get().transition_to(GameStateT::Lobby),
             "transition_to Lobby from InMenu should be legal");
    VALIDATE(GameState::get().is(GameStateT::Lobby),
             "transition_to should apply the new state");
    GameState::get().reset();
}

}  // namespace tests
