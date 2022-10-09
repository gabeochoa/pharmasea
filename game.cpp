
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

int main(void) {
    InitWindow(WIN_W, WIN_H, "pharmasea");
    App app;

    // Disable global esc to close window
    SetExitKey(KEY_NULL);

    GameLayer* gamelayer = new GameLayer();
    app.pushLayer(gamelayer);

    MenuLayer* menulayer = new MenuLayer();
    app.pushLayer(menulayer);

    
    Menu::get().state = Menu::State::Game;
    // Menu::get().state = Menu::State::Root;


    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        app.run(dt);
    }

    CloseWindow();

    return 0;
}
