#pragma once

#include "drawing_util.h"
#include "event.h"
#include "external_include.h"
//
#include "globals.h"
//
#include "camera.h"
#include "files.h"
#include "layer.h"
#include "menu.h"
#include "model_library.h"
#include "music_library.h"
#include "raylib.h"
#include "texture_library.h"
#include "ui_color.h"

struct GameLayer : public Layer {
    std::shared_ptr<Player> player;
    std::shared_ptr<GameCam> cam;
    Model bag_model;

    GameLayer() : Layer("Game") {
        minimized = false;

        player.reset(new Player(vec2{-3, -3}));
        GLOBALS.set("player", player.get());
        EntityHelper::addEntity(player);

        cam.reset(new GameCam());
        GLOBALS.set("game_cam", cam.get());
    }

    virtual ~GameLayer() {}
    virtual void onAttach() override {}
    virtual void onDetach() override {}

    virtual void onEvent(Event& event) override {
        if (Menu::get().state != Menu::State::Game) return;
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<KeyPressedEvent>(
            std::bind(&GameLayer::onKeyPressed, this, std::placeholders::_1));
        dispatcher.dispatch<GamepadButtonPressedEvent>(std::bind(
            &GameLayer::onGamepadButtonPressed, this, std::placeholders::_1));
        dispatcher.dispatch<GamepadAxisMovedEvent>(std::bind(
            &GameLayer::onGamepadAxisMoved, this, std::placeholders::_1));
    }

    bool onGamepadAxisMoved(GamepadAxisMovedEvent&) { return false; }

    bool onGamepadButtonPressed(GamepadButtonPressedEvent& event) {
        if (KeyMap::get_button(Menu::State::Game, "Pause") == event.button) {
            Menu::get().state = Menu::State::Paused;
            return true;
        }
        return false;
    }

    bool onKeyPressed(KeyPressedEvent& event) {
        if (KeyMap::get_key_code(Menu::State::Game, "Pause") == event.keycode) {
            Menu::get().state = Menu::State::Paused;
            return true;
        }
        return false;
    }

    void play_music() {
        auto m = MusicLibrary::get().get("wah");
        if (!IsMusicStreamPlaying(m)) {
            PlayMusicStream(m);
        }
        UpdateMusicStream(m);
    }

    virtual void onUpdate(float dt) override {
        if (Menu::get().state != Menu::State::Game) return;
        if (minimized) return;
        PROFILE();

        play_music();

        // Dont quit window on escape
        SetExitKey(KEY_NULL);

        // TODO replace passing Player with passing Player*
        // update
        cam->updateToTarget(GLOBALS.get<Player>("player"));
        cam->updateCamera();

        EntityHelper::forEachEntity([&](auto entity) {
            entity->update(dt);
            return EntityHelper::ForEachFlow::None;
        });
    }

    void render_entities() {
        PROFILE();
        EntityHelper::forEachEntity([&](auto entity) {
            entity->render();
            return EntityHelper::ForEachFlow::None;
        });
    }

    virtual void onDraw(float) override {
        if (Menu::get().state != Menu::State::Game) return;
        if (minimized) return;
        PROFILE();

        ClearBackground(Color{200, 200, 200, 255});
        BeginMode3D((*cam).get());
        {
            render_entities();

            EntityHelper::forEachItem([&](auto item) {
                item->render();
                return EntityHelper::ForEachFlow::None;
            });

            // DrawGrid(40, TILESIZE);

            DrawBillboard(cam->camera, TextureLibrary::get().get("face"),
                          {
                              1.f,
                              0.f,
                              1.f,
                          },
                          TILESIZE, WHITE);

            if (GLOBALS.get<bool>("debug_ui_enabled")) {
                auto nav = GLOBALS.get_ptr<NavMesh>("navmesh");
                if (nav) {
                    for (auto kv : nav->entityShapes) {
                        DrawLineStrip2Din3D(kv.second.hull, PINK);
                    }

                    for (auto kv : nav->shapes) {
                        DrawLineStrip2Din3D(kv.hull, PINK);
                    }
                }
            }
        }
        EndMode3D();
    }
};
