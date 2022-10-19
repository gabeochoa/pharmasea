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
#include "raylib.h"
#include "ui_color.h"
#include "world.h"
// temporary for face cube test
#include "texture_library.h"
#include "modellibrary.h"

struct GameLayer : public Layer {
    World world;
    GameCam cam;
    Model bag_model;
    // TODO move to pharmacy.h
    bool in_planning_mode = false;

    GameLayer() : Layer("Game") {
        minimized = false;
        GLOBALS.set("game_cam", &cam);
        GLOBALS.set("in_planning", &in_planning_mode);
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

    bool onGamepadAxisMoved(GamepadAxisMovedEvent&) {
        return false;
    }

    bool onGamepadButtonPressed(GamepadButtonPressedEvent& event) {
        if (KeyMap::get_button(Menu::State::Game, "Pause") == event.button) {
            Menu::get().state = Menu::State::Root;
            return true;
        }
        if (KeyMap::get_button(Menu::State::Game,
                                 "Toggle Planning [Debug]") == event.button) {
            in_planning_mode = !in_planning_mode;
            return true;
        }
        return false;
    }

    bool onKeyPressed(KeyPressedEvent& event) {
        if (KeyMap::get_key_code(Menu::State::Game, "Pause") == event.keycode) {
            Menu::get().state = Menu::State::Root;
            return true;
        }
        // TODO remove once we have gameplay loop
        if (KeyMap::get_key_code(Menu::State::Game,
                                 "Toggle Planning [Debug]") == event.keycode) {
            in_planning_mode = !in_planning_mode;
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

            // DrawGrid(40, TILESIZE);

            DrawBillboard(cam.camera, TextureLibrary::get().get("face"),
                          {
                              1.f,
                              0.f,
                              1.f,
                          },
                          TILESIZE, WHITE);

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
        EndMode3D();

        if (in_planning_mode) {
            DrawTextEx(Preload::get().font, "IN PLANNING MODE", vec2{100, 100}, 20,
                       0, RED);
        }
    }
};
