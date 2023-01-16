
#pragma once

#include <iostream>
#include <stack>

#include "engine/log.h"
//
#include "engine/singleton.h"

// TODO eventually extract this piece to engine
template<typename T>
struct StateManager {
    [[nodiscard]] T read() const { return state; }

    T go_back() {
        log_trace("going from {} to {}", state, prev.top());
        state = prev.top();
        prev.pop();
        return state;
    }

    void clear_history() {
        while (!prev.empty()) prev.pop();
    }

    [[nodiscard]] bool is(T s) const { return state == s; }
    [[nodiscard]] bool is_not(T s) const { return state != s; }
    [[nodiscard]] inline const std::string_view tostring() const {
        return magic_enum::enum_name(state);
    }

    void set(T ns) {
        if (state == ns) return;
        log_trace("trying to set state to {} (was {})", ns, state);

        prev.push(state);
        state = ns;

        // Note: this is just for dev to figure out if
        //       we have any issues in our logic that allows circular visits
        if (prev.size() >= 10) {
            log_warn("prev state getting large: {}", prev.size());
            // TODO at some point we could condense the results if there is a
            // loop
        }
    }

    virtual T default_value() const = 0;

    void reset() {
        clear_history();
        set(default_value());
    }

    virtual ~StateManager() {}

   private:
    std::stack<T> prev;
    T state;
};

namespace menu {
enum struct State {
    Root = 0,
    About = 1,
    Network = 2,
    Settings = 3,
    Game = 4,

    UI = 99,
};

inline std::ostream& operator<<(std::ostream& os, const State& state) {
    os << "Menu::State " << magic_enum::enum_name(state);
    return os;
}

}  // namespace menu

SINGLETON_FWD(MenuState)
struct MenuState : public StateManager<menu::State> {
    SINGLETON(MenuState)

    virtual menu::State default_value() const override {
        return menu::State::Root;
    }

    [[nodiscard]] bool is_in_menu() const { return is_not(menu::State::Game); }
    [[nodiscard]] bool in_game() const { return is(menu::State::Game); }

    [[nodiscard]] static bool s_in_game() { return MenuState::get().in_game(); }
    [[nodiscard]] static bool s_is_game(menu::State ms) {
        return ms == menu::State::Game;
    }
};

namespace game {
enum State {
    Lobby = 0,
    InRound = 1,
    Planning = 2,
    Paused = 3,
};
inline std::ostream& operator<<(std::ostream& os, const State& state) {
    os << "Game::State " << magic_enum::enum_name(state);
    return os;
}

}  // namespace game

SINGLETON_FWD(GameState)
struct GameState : public StateManager<game::State> {
    SINGLETON(GameState)

    virtual game::State default_value() const override {
        return game::State::Lobby;
    }

    game::State pause() {
        if (is_paused()) {
            return go_back();
        }
        set(game::State::Paused);
        return read();
    }
    static game::State s_pause() { return GameState::get().pause(); }

    [[nodiscard]] bool in_round() const { return is(game::State::InRound); }
    [[nodiscard]] static bool s_in_round() {
        return GameState::get().in_round();
    }

    [[nodiscard]] bool is_paused() const { return is(game::State::Paused); }
    [[nodiscard]] static bool s_is_paused() {
        return GameState::get().is_paused();
    }

    [[nodiscard]] static bool s_should_update() {
        const auto s = GameState::get().read();
        return s == game::State::InRound || s == game::State::Planning;
    }
    [[nodiscard]] static bool s_should_draw() { return s_should_update(); }

    game::State toggle_planning() {
        // TODO need logic here to stop loops
        if (is(game::State::Planning)) {
            set(game::State::InRound);
        } else {
            set(game::State::Planning);
        }
        return read();
    }
    static game::State s_toggle_planning() {
        return GameState::get().toggle_planning();
    }
};
