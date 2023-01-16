#pragma once

#include "drawing_util.h"
#include "engine/ui_color.h"
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
#include "player.h"
#include "statemanager.h"

struct GameLayer : public Layer {
    std::shared_ptr<Player> player;
    std::shared_ptr<BasePlayer> active_player;
    std::shared_ptr<GameCam> cam;
    raylib::Model bag_model;

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
        if (!MenuState::s_in_game()) return;
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
        if (KeyMap::get_button(menu::State::Game, InputName::Pause) ==
            event.button) {
            GameState::s_pause();
            return true;
        }
        return false;
    }

    bool onKeyPressed(KeyPressedEvent& event) {
        // Note: You can only pause in game state, in planning no pause
        if (KeyMap::get_key_code(menu::State::Game, InputName::Pause) ==
            event.keycode) {
            GameState::s_pause();
            //  TODO obv need to have this fun on a timer or something instead
            //  of on esc
            // SoundLibrary::get().play_random_match("pa_announcements_");
            return true;
        }
        return false;
    }

    void play_music() {
        auto m = MusicLibrary::get().get("supermarket");
        if (!IsMusicStreamPlaying(m)) {
            PlayMusicStream(m);
        }
        UpdateMusicStream(m);
    }

    virtual void onUpdate(float dt) override {
        TRACY_ZONE_SCOPED;
        if (MenuState::s_in_game()) play_music();

        if (!GameState::s_should_update()) return;

        // Dont quit window on escape
        raylib::SetExitKey(raylib::KEY_NULL);

        // TODO Why does updateCamera not just take an optional target?
        cam->updateToTarget(
            GLOBALS.get_or_default<Entity>("active_camera_target", *player));
        cam->updateCamera();

        player->update(dt);
        // NOTE: gabe: i dont think we need to do this
        //         because the local map never should need to grab things
        //         TODO do we?
        auto map_ptr = GLOBALS.get_ptr<Map>("map");
        if (map_ptr) {
            map_ptr->grab_things();
            map_ptr->onUpdate(dt);
        }
    }

    virtual void onDraw(float dt) override {
        TRACY_ZONE_SCOPED;
        if (!MenuState::s_in_game()) return;

        const auto map_ptr = GLOBALS.get_ptr<Map>("map");

        ext::clear_background(Color{200, 200, 200, 255});
        raylib::BeginMode3D((*cam).get());
        {
            raylib::DrawPlane((vec3){0.0f, -TILESIZE, 0.0f},
                              (vec2){256.0f, 256.0f}, DARKGRAY);
            if (map_ptr) map_ptr->onDraw(dt);
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
        raylib::EndMode3D();

        // note: for ui stuff
        if (map_ptr) map_ptr->onDrawUI(dt);
    }
};
