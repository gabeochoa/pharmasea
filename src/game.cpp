
#include "external_include.h"

///
#include "globals.h"
///

#include "camera.h"
#include "world.h"
#include "menu.h"
#include "app.h"
#include "gamelayer.h"
#include "menulayer.h"
#include "aboutlayer.h"
#include "menustatelayer.h"

int main(void) {
    App app = App::get();

    GameLayer* gamelayer = new GameLayer();
    app.pushLayer(gamelayer);

    AboutLayer* aboutlayer = new AboutLayer();
    app.pushLayer(aboutlayer);

    MenuLayer* menulayer = new MenuLayer();
    app.pushLayer(menulayer);

    MenuStateLayer* menustatelayer = new MenuStateLayer();
    app.pushLayer(menustatelayer);

    // Menu::get().state = Menu::State::Game;
    Menu::get().state = Menu::State::Root;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        app.run(dt);
    }

    CloseWindow();

    return 0;
}
