
#include "external_include.h"

// Global Defines

// uncomment to enable writing / reading files :
// settings file 
// save file
// #define WRITE_FILES

///
#include "globals.h"
///

#include "app.h"
#include "menu.h"
#include "settings.h"
//
#include "aboutlayer.h"
#include "gamelayer.h"
#include "menulayer.h"
#include "menustatelayer.h"
#include "settingslayer.h"

void startup() {
    // Disable all that startup logging
    SetTraceLogLevel(LOG_WARNING);


    // Menu::get().state = Menu::State::Game;
    Menu::get().state = Menu::State::Root;
    // Menu::get().state = Menu::State::Settings;

    GameLayer* gamelayer = new GameLayer();
    App::get().pushLayer(gamelayer);

    AboutLayer* aboutlayer = new AboutLayer();
    App::get().pushLayer(aboutlayer);

    SettingsLayer* settingslayer = new SettingsLayer();
    App::get().pushLayer(settingslayer);

    MenuLayer* menulayer = new MenuLayer();
    App::get().pushLayer(menulayer);

    MenuStateLayer* menustatelayer = new MenuStateLayer();
    App::get().pushLayer(menustatelayer);

    Settings::get().load_save_file();
}

void teardown() {
#ifdef WRITE_FILES
    Settings::get().write_save_file();
#endif
}

int main(void) {
    startup();
    App::get().run();
    teardown();
    return 0;
}
