#pragma once

#include "../components/can_be_ghost_player.h"
#include "../drawing_util.h"
#include "../engine/ui/color.h"
#include "../external_include.h"
//
#include "../globals.h"
//
#include "../camera.h"
#include "../engine.h"
#include "../map.h"
#include "raylib.h"

struct GameLayer : public Layer {
    std::shared_ptr<Entity> active_player;
    std::shared_ptr<GameCam> cam;
    raylib::Model bag_model;
    raylib::RenderTexture2D game_render_texture;

    GameLayer() : Layer(strings::menu::GAME), cam(std::make_shared<GameCam>()) {
        GLOBALS.set(strings::globals::GAME_CAM, cam.get());
        game_render_texture = raylib::LoadRenderTexture(WIN_W(), WIN_H());
    }

    virtual ~GameLayer() { raylib::UnloadRenderTexture(game_render_texture); }

    bool onGamepadAxisMoved(GamepadAxisMovedEvent&) override { return false; }

    bool onGamepadButtonPressed(GamepadButtonPressedEvent& event) override {
        if (!MenuState::s_in_game()) return false;
        if (KeyMap::get_button(menu::State::Game, InputName::Pause) ==
            event.button) {
            GameState::get().pause();
            return true;
        }
        return false;
    }

    bool onKeyPressed(KeyPressedEvent& event) override {
        if (!MenuState::s_in_game()) return false;
        // Note: You can only pause in game state, in planning no pause
        if (KeyMap::get_key_code(menu::State::Game, InputName::Pause) ==
            event.keycode) {
            GameState::get().pause();
            //  TODO obv need to have this fun on a timer or something instead
            //  of on esc
            // SoundLibrary::get().play_random_match("pa_announcements_");
            return true;
        }
        return false;
    }

    void play_music() {
        if (ENABLE_SOUND) {
            auto m = MusicLibrary::get().get(strings::music::SUPERMARKET);
            if (!IsMusicStreamPlaying(m)) {
                PlayMusicStream(m);
            }
            UpdateMusicStream(m);
        }
    }

    virtual void onUpdate(float dt) override {
        TRACY_ZONE_SCOPED;
        if (MenuState::s_in_game()) play_music();

        if (!GameState::get().should_update()) return;

        // Dont quit window on escape
        raylib::SetExitKey(raylib::KEY_NULL);

        auto act = GLOBALS.get_ptr<Entity>(strings::globals::CAM_TARGET);
        if (!act) {
            return;
        }

        cam->updateToTarget(act->get<Transform>().pos());
        cam->updateCamera();

        //         jun 24-23 we need this so furniture shows up
        auto map_ptr = GLOBALS.get_ptr<Map>(strings::globals::MAP);
        if (map_ptr) {
            // NOTE: today we need to grab things so that the client renders
            // what they server has access to
            map_ptr->grab_things();
            map_ptr->onUpdateLocalPlayers(dt);
        }
    }

    void draw_world(float dt) {
        auto map_ptr = GLOBALS.get_ptr<Map>(strings::globals::MAP);
        const auto network_debug_mode_on =
            GLOBALS.get_or_default<bool>("network_ui_enabled", false);
        if (network_debug_mode_on) {
            map_ptr = GLOBALS.get_ptr<Map>("server_map");
        }

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
    }

    virtual void onDraw(float dt) override {
        TRACY_ZONE_SCOPED;
        if (!MenuState::s_in_game()) return;

        auto map_ptr = GLOBALS.get_ptr<Map>(strings::globals::MAP);

        ext::clear_background(Color{200, 200, 200, 255});
        draw_world(dt);

        // note: for ui stuff
        if (map_ptr) map_ptr->onDrawUI(dt);
    }
};
