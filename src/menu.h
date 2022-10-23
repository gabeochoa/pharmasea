
#pragma once

#include <iostream>
#include "singleton.h"

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

        UI = 98,
        Any = 99,
    };

    State state;

    inline const char* tostring() {
        switch (state) {
            case Menu::State::About:
                return "About";
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
            case Menu::State::Network:
                return "Network";
            default:
                std::cout << "Invalid state" << std::endl;
                return "MenuState no stateToString";
        }
    }
};
