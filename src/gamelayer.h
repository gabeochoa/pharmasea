#pragma once

#include "drawing_util.h"
#include "external_include.h"
//
#include "globals.h"
//
#include "camera.h"
#include "engine.h"
#include "engine/layer.h"
#include "engine/model_library.h"
#include "engine/music_library.h"
#include "engine/texture_library.h"
#include "map.h"
#include "menu.h"
#include "player.h"
#include "raylib.h"

struct GameLayer : public Layer {
    std::shared_ptr<Player> player;
    std::shared_ptr<BasePlayer> active_player;
    std::shared_ptr<GameCam> cam;
    Model bag_model;

    GameLayer() : Layer("Game") {
        player.reset(new Player(vec2{-3, -3}));
        GLOBALS.set("player", player.get());

        // TODO do we need this still?
        player->is_ghost_player = true;

        cam.reset(new GameCam());
        GLOBALS.set("game_cam", cam.get());
    }

    virtual ~GameLayer() {}

    virtual void onEvent(Event& event) override {
        if (!Menu::in_game()) return;
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
        // Note: You can only pause in game state, in planning no pause
        if (KeyMap::get_button(Menu::State::Game, InputName::Pause) ==
            event.button) {
            Menu::pause();
            return true;
        }
        return false;
    }

    bool onKeyPressed(KeyPressedEvent& event) {
        // Note: You can only pause in game state, in planning no pause
        if (KeyMap::get_key_code(Menu::State::Game, InputName::Pause) ==
            event.keycode) {
            Menu::pause();
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
        if (!Menu::in_game()) return;
        PROFILE();

        play_music();

        // Dont quit window on escape
        SetExitKey(KEY_NULL);

        // TODO Why does updateCamera not just take an optional target?
        cam->updateToTarget(
            GLOBALS.get_or_default<Entity>("active_camera_target", *player));
        cam->updateCamera();

        player->update(dt);
        // NOTE: gabe: i dont think we need to do this
        //         because the local map never should need to grab things
        auto map_ptr = GLOBALS.get_ptr<Map>("map");
        if (map_ptr) {
            map_ptr->grab_things();
            map_ptr->onUpdate(dt);
        }
    }

    virtual void onDraw(float dt) override {
        if (!Menu::in_game() && !Menu::is_paused()) return;
        PROFILE();

        ClearBackground(Color{200, 200, 200, 255});
        BeginMode3D((*cam).get());
        {
            auto map_ptr = GLOBALS.get_ptr<Map>("map");
            if (map_ptr) map_ptr->onDraw(dt);

            // TODO migrate
            ItemHelper::forEachItem([&](auto item) {
                item->render();
                return ItemHelper::ForEachFlow::None;
            });

            // DrawGrid(40, TILESIZE);

            DrawBillboard(cam->camera, TextureLibrary::get().get("face"),
                          {
                              1.f,
                              0.f,
                              1.f,
                          },
                          TILESIZE, WHITE);

            // if (GLOBALS.get_or_default<bool>("network_debug_enabled", false))
            // { auto host_map_ptr = GLOBALS.get_ptr<Map>("server_map"); if
            // (host_map_ptr) {
            // host_map_ptr->onDraw(dt);
            // }
            // }

            // auto nav = GLOBALS.get_ptr<NavMesh>("navmesh");
            // if (nav) {
            // for (auto kv : nav->entityShapes) {
            // DrawLineStrip2Din3D(kv.second.hull, PINK);
            // }
            //
            // for (auto kv : nav->shapes) {
            // DrawLineStrip2Din3D(kv.hull, PINK);
            // }
            // }
        }
        EndMode3D();
    }
};
