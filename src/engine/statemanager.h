
#pragma once

#include <iostream>
#include <stack>

#include "log.h"
#include "singleton.h"

// TODO eventually extract this piece to engine
template<typename T>
struct StateManager2 {
    [[nodiscard]] T read() const { return state; }
    [[nodiscard]] T previous() const { return prev.top(); }

    T go_back() {
        log_info("going from {} to {}", state, prev.top());
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

    void register_on_change(const std::function<void(T, T)>& nfunc) {
        on_change_action_list.push_back(nfunc);
    }

    void set(T ns) {
        if (state == ns) return;
        log_info("trying to set state to {} (was {})", ns, state);
        prev.push(state);
        state = ns;

        // Note: this is just for dev to figure out if
        //       we have any issues in our logic that allows circular visits
        if (prev.size() >= 10) {
            log_warn("prev state getting large: {}", prev.size());
            // TODO at some point we could condense the results if there is a
            // loop
        }

        call_on_change(ns, prev.top());
    }

    virtual T default_value() const = 0;

    void reset() {
        clear_history();
        set(default_value());
    }

    virtual ~StateManager2() {}

   private:
    void call_on_change(T ns, T os) {
        for (const auto& func : on_change_action_list) {
            if (func) func(ns, os);
        }
    }

    std::stack<T> prev;
    std::vector<std::function<void(T, T)>> on_change_action_list;
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
struct MenuState : public StateManager2<menu::State> {
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
    InMenu = 0,
    Lobby = 1,
    InRound = 2,
    Planning = 3,
    Paused = 4,
    Progression = 5,
};
inline std::ostream& operator<<(std::ostream& os, const State& state) {
    os << "Game::State " << magic_enum::enum_name(state);
    return os;
}

}  // namespace game

SINGLETON_FWD(GameState)
struct GameState : public StateManager2<game::State> {
    SINGLETON(GameState)

    virtual game::State default_value() const override {
        return game::State::InMenu;
    }

    game::State pause() {
        if (is_paused()) {
            return go_back();
        }
        set(game::State::Paused);
        return read();
    }

    [[nodiscard]] bool in_round() const { return is(game::State::InRound); }
    [[nodiscard]] bool in_planning() const { return is(game::State::Planning); }

    [[nodiscard]] bool is_paused() const { return is(game::State::Paused); }

    [[nodiscard]] bool is_paused_in(game::State s) const {
        return is(game::State::Paused) && previous() == s;
    }

    [[nodiscard]] bool is_update_state(game::State s) const {
        return s == game::State::Lobby ||        //
               s == game::State::InRound ||      //
               s == game::State::Progression ||  //
               s == game::State::Planning;
    }

    [[nodiscard]] bool should_update() const { return is_update_state(read()); }

    // Basically we want to know if the prev state was an updatable state
    // this is so that if we are paused we can still render whats underneath
    [[nodiscard]] bool should_prev_update() const {
        if (read() != game::State::Paused) return false;
        return is_update_state(previous());
    }

    [[nodiscard]] bool is_lobby_like() const {
        return is(game::State::Lobby);  // TODO this is needed to get collisions
                                        // in lobby mode but breaks game
                                        // furniture loading for some reason
                                        //|| is(game::State::InMenu);
    }
    [[nodiscard]] bool is_game_like() const {
        return is(game::State::InRound) || is(game::State::Planning) ||
               is(game::State::Progression);
    }

    [[nodiscard]] bool should_render_timer() const {
        return is(game::State::InRound) || is(game::State::Planning);
    }

    game::State toggle_planning() {
        // TODO need logic here to stop loops
        if (is(game::State::Planning)) {
            set(game::State::InRound);
        } else if (is(game::State::InRound)) {
            set(game::State::Planning);
        }
        return read();
    }

    void toggle_to_planning() { return set(game::State::Planning); }
    void toggle_to_inround() { return set(game::State::InRound); }
};
