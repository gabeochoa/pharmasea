
#include "external_include.h"

///
#include "globals.h"
///

#include "aboutlayer.h"
#include "app.h"
#include "camera.h"
#include "gamelayer.h"
#include "menu.h"
#include "menulayer.h"
#include "menustatelayer.h"
#include "settingslayer.h"
#include "world.h"

int main(void) {
    App app = App::get();

    GameLayer* gamelayer = new GameLayer();
    app.pushLayer(gamelayer);

    AboutLayer* aboutlayer = new AboutLayer();
    app.pushLayer(aboutlayer);

    SettingsLayer* settingslayer = new SettingsLayer();
    app.pushLayer(settingslayer);

    MenuLayer* menulayer = new MenuLayer();
    app.pushLayer(menulayer);

    MenuStateLayer* menustatelayer = new MenuStateLayer();
    app.pushLayer(menustatelayer);

    Menu::get().state = Menu::State::Game;
    // Menu::get().state = Menu::State::Root;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        app.run(dt);
    }

    CloseWindow();

    return 0;
}
