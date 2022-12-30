
#pragma once

#include <iostream>
#include <stack>

#include "engine/log.h"
#include "engine/singleton.h"

SINGLETON_FWD(Menu)
struct Menu {
    SINGLETON(Menu)
    enum State {
        Root = 0,
        Game = 1,
        Paused = 2,
        Settings = 3,
        About = 4,
        Network = 5,
        Planning = 6,
        PausedPlanning = 7,

        UI = 98,
        Any = 99,
    };

   private:
    std::stack<State> prev;
    State state;

   public:
    void set(State ns) {
        if (state == ns) return;

        prev.push(state);
        state = ns;

        // Note: this is just for dev to figure out if
        //       we have any issues in our logic that allows circular visits
        if (prev.size() >= 10) {
            log_warn("prev state getting large: {}", prev.size());
        }
    }

    State read() const { return state; }
    State go_back() {
        state = prev.top();
        prev.pop();
        return state;
    }
    void clear_history() {
        while (!prev.empty()) prev.pop();
    }
    bool is(State s) const { return state == s; }
    bool is_not(State s) const { return state != s; }

    static bool in_game(Menu::State state) {
        return state == Menu::State::Game || state == Menu::State::Planning;
    }
    static bool in_game() { return Menu::in_game(Menu::get().state); }

    static bool is_paused(Menu::State state) {
        return state == Menu::State::Paused ||
               state == Menu::State::PausedPlanning;
    }
    static bool is_paused() { return Menu::is_paused(Menu::get().state); }

    static void toggle_planning() {
        if (Menu::get().state == Menu::State::Game) {
            Menu::get().set(Menu::State::Planning);
        } else if (Menu::get().state == Menu::State::Planning) {
            Menu::get().set(Menu::State::Game);
        }
    }

    // TODO do we need paused planning anymore? if we use goback instead?
    static void pause() {
        switch (Menu::get().state) {
            case Menu::State::Game:
                Menu::get().set(Menu::State::Paused);
                break;
            case Menu::State::Planning:
                Menu::get().set(Menu::State::PausedPlanning);
                break;
            default:
                break;
        }
    }

    inline const char* tostring() {
        switch (state) {
            case Menu::State::About:
                return "About";
            case Menu::State::Root:
                return "Root";
            case Menu::State::Game:
                return "Game";
            case Menu::State::Planning:
                return "Planning";
            case Menu::State::Paused:
                return "Paused";
            case Menu::State::PausedPlanning:
                return "PausedPlanning";
            case Menu::State::UI:
                return "UI";
            case Menu::State::Settings:
                return "Settings";
            case Menu::State::Network:
                return "Network";
            default:
                log_warn("no stateToString, Invalid state: {}", state);
                return "MenuState no stateToString";
        }
    }
};
