
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

void folder_locations (){
    std::cout << "Config: " << sago::getConfigHome() << "\n";
    std::cout << "Data: " << sago::getDataHome() << "\n";
    std::cout << "State: " << sago::getStateDir() << "\n";
    std::cout << "Cache: " << sago::getCacheDir() << "\n";
    std::cout << "Documents: " << sago::getDocumentsFolder() << "\n";
    std::cout << "Desktop: " << sago::getDesktopFolder() << "\n";
    std::cout << "Pictures: " << sago::getPicturesFolder() << "\n";
    std::cout << "Public: " << sago::getPublicFolder() << "\n";
    std::cout << "Music: " << sago::getMusicFolder() << "\n";
    std::cout << "Video: " << sago::getVideoFolder() << "\n";
    std::cout << "Download: " << sago::getDownloadFolder() << "\n";
    std::cout << "Save Games 1: " << sago::getSaveGamesFolder1() << "\n";
    std::cout << "Save Games 2: " << sago::getSaveGamesFolder2() << "\n";
    std::vector<std::string> extraData;
    sago::appendAdditionalDataDirectories(extraData);
    for (size_t i = 0; i < extraData.size(); ++i) {
        std::cout << "Additional data " << i << ": " << extraData.at(i) << "\n";
    }
}

int main(void) {
    folder_locations();

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

    // Menu::get().state = Menu::State::Game;
    Menu::get().state = Menu::State::Root;
    // Menu::get().state = Menu::State::Settings;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        app.run(dt);
    }

    CloseWindow();

    return 0;
}
