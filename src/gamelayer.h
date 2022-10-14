#pragma once

#include "event.h"
#include "external_include.h"
//
#include "globals.h"
//
#include "camera.h"
#include "files.h"
#include "layer.h"
#include "menu.h"
#include "worldgen.h"
// temporary for face cube test
#include "texture_library.h"

struct GameLayer : public Layer {
    // TODO support loading the world from a save and deserializing the savegame
    // Generate the world and starting entities.
    WorldGen world;
    GameCam cam;

    GameLayer() : Layer("Game") {

        minimized = false;
        GLOBALS.set("game_cam", &cam);
        preload_textures();
    }
    virtual ~GameLayer() {}
    virtual void onAttach() override {}
    virtual void onDetach() override {}

    void preload_textures() {
        TextureLibrary::get().load(
            Files::get().fetch_resource_path("images", "face.png").c_str(),
            "face");

        TextureLibrary::get().load(
            Files::get().fetch_resource_path("images", "jug.png").c_str(),
            "jug");

        TextureLibrary::get().load(
            Files::get().fetch_resource_path("images", "sleepyico.png").c_str(),
            "bubble");
    }

    virtual void onEvent(Event& event) override {
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<KeyPressedEvent>(
            std::bind(&GameLayer::onKeyPressed, this, std::placeholders::_1));
        dispatcher.dispatch<GamepadButtonPressedEvent>(std::bind(
            &GameLayer::onGamepadButtonPressed, this, std::placeholders::_1));
    }

    bool onGamepadButtonPressed(GamepadButtonPressedEvent& event) {
        if (KeyMap::get_button(Menu::State::Game, "Pause") == event.button) {
            Menu::get().state = Menu::State::Root;
            return true;
        }
        return false;
    }

    bool onKeyPressed(KeyPressedEvent& event) {
        if (KeyMap::get_key_code(Menu::State::Game, "Pause") == event.keycode) {
            Menu::get().state = Menu::State::Root;
            return true;
        }
        return false;
    }

    virtual void onUpdate(float dt) override {
        if (Menu::get().state != Menu::State::Game) return;
        if (minimized) return;
        // Dont quit window on escape
        SetExitKey(KEY_NULL);

        // TODO replace passing Player with passing Player*
        // update
        cam.updateToTarget(GLOBALS.get<Player>("player"));
        cam.updateCamera();

        EntityHelper::forEachEntity([&](auto entity) {
            entity->update(dt);
            return EntityHelper::ForEachFlow::None;
        });
    }

    virtual void onDraw(float) override {
        if (Menu::get().state != Menu::State::Game) return;
        if (minimized) return;

        ClearBackground(Color{200, 200, 200, 255});
        BeginMode3D(cam.get());
        {
            EntityHelper::forEachEntity([&](auto entity) {
                entity->render();
                return EntityHelper::ForEachFlow::None;
            });

            EntityHelper::forEachItem([&](auto item) {
                item->render();
                return EntityHelper::ForEachFlow::None;
            });

            DrawGrid(40, TILESIZE);

            DrawBillboard(cam.camera, TextureLibrary::get().get("face"),
                          {
                              1.f,
                              0.f,
                              1.f,
                          },
                          TILESIZE, WHITE);
        }
        EndMode3D();
    }
};
