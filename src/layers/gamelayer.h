#pragma once

#include "../components/can_be_ghost_player.h"
#include "../drawing_util.h"
#include "../engine/ui_color.h"
#include "../external_include.h"
//
#include "../globals.h"
//
#include "../camera.h"
#include "../engine.h"
#include "../map.h"

struct GameLayer : public Layer {
    // TODO eventually we should not have this
    // this can be done by adding the input components to remote_player
    // and then setting GLOBALS on the PlayerJoin.is_you packet
    //
    // The issue with this is that the non-host player no longer sees
    // any furniture for some reason. Though they can still collide against it
    // (since its server side)
    //
    std::shared_ptr<Entity> player;
    std::shared_ptr<Entity> active_player;
    std::shared_ptr<GameCam> cam;
    raylib::Model bag_model;

    GameLayer() : Layer("Game") {
        player.reset(make_player(vec3{-3, 0, -3}));
        GLOBALS.set("player", player.get());

        // TODO do we need this still?
        // -> jun24-23 it seems like we do need it for debug view only
        player->get<CanBeGhostPlayer>().update(true);

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

        auto act = GLOBALS.get_ptr<Entity>("active_camera_target");
        if (act) {
            cam->updateToTarget(act->get<Transform>().pos());
        }
        cam->updateCamera();

        SystemManager::get().update(Entities{player}, dt);

        // NOTE: gabe: i dont think we need to do this
        //         because the local map never should need to grab things
        //         TODO do we?
        auto map_ptr = GLOBALS.get_ptr<Map>("map");
        if (map_ptr) {
            // NOTE: today we need to grab things so that the client renders
            // what they server has access to
            map_ptr->grab_things();

            // TODO we need this so the entity -> floating name moves around
            // why does that happen
            map_ptr->onUpdate(dt);
        }
    }

    virtual void onDraw(float dt) override {
        TRACY_ZONE_SCOPED;
        if (!MenuState::s_in_game()) return;

        auto map_ptr = GLOBALS.get_ptr<Map>("map");
        const auto network_debug_mode_on =
            GLOBALS.get_or_default<bool>("network_ui_enabled", false);
        if (network_debug_mode_on) {
            map_ptr = GLOBALS.get_ptr<Map>("server_map");
        }

        ext::clear_background(Color{200, 200, 200, 255});
        raylib::BeginMode3D((*cam).get());
        {
            raylib::DrawPlane((vec3){0.0f, -TILESIZE, 0.0f},
                              (vec2){256.0f, 256.0f}, DARKGRAY);
            if (map_ptr) map_ptr->onDraw(dt);
            if (network_debug_mode_on) {
                const auto network_players =
                    // Forcing a copy to avoid the data changing from underneath
                    // us
                    std::map<int, std::shared_ptr<Entity>>(
                        GLOBALS.get_or_default<
                            std::map<int, std::shared_ptr<Entity>>>(
                            "server_players", {}));
                SystemManager::get().render_entities(Entities{player}, dt);
            }
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
