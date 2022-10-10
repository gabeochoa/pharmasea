
#pragma once

#include "external_include.h"

struct Menu;
static std::shared_ptr<Menu> menu;

struct Menu {
    enum State {
        Root = 0,
        Game = 1,
        Paused = 2,
        Settings = 3,
        About = 4,

        UI = 98,
        Any = 99,
    };

    State state;
    inline static Menu* create() { return new Menu(); }
    inline static Menu& get() {
        if (!menu) menu.reset(Menu::create());
        return *menu;
    }

    inline const char* tostring() {
        switch (state) {
            case Menu::State::Root:
                return "Root";
            case Menu::State::Game:
                return "Game";
            case Menu::State::Paused:
                return "Paused";
            case Menu::State::UI:
                return "UI";
            case Menu::State::Settings:
                return "Settings";
            case Menu::State::About:
                return "About";
            default:
                std::cout << "Invalid state" << std::endl;
                return "MenuState no stateToString";
        }
    }
};
