#pragma once

#include "../components/can_be_ghost_player.h"
#include "../drawing_util.h"
#include "../engine/ui/color.h"
#include "../external_include.h"
//
#include "../globals.h"
//
#include "../camera.h"
#include "../dataclass/names.h"
#include "../engine.h"
#include "../engine/layer.h"
#include "../map.h"
#include "raylib.h"

struct SeedManagerLayer : public Layer {
    std::shared_ptr<GameCam> cam;
    std::shared_ptr<ui::UIContext> ui_context;
    bool showSeedInputBox = false;
    // We use a temp string because we dont want to touch the real one until the
    // user says Okay
    std::string tempSeed = "";
    Layer* network;

    SeedManagerLayer()
        : Layer(strings::menu::GAME),
          cam(std::make_shared<GameCam>()),
          ui_context(std::make_shared<ui::UIContext>()) {
        cam->updateToTarget({0, -90, 0});
        cam->free_distance_min_clamp = -10.0f;
        cam->free_distance_max_clamp = 200.0f;
    }

    virtual ~SeedManagerLayer() {}

    bool is_user_host() {
        if (network != nullptr) {
            // TODO read from network::Info
        }
        return true;
    }

    bool onKeyPressed(KeyPressedEvent& event) override {
        if (GameState::get().is(game::State::Paused)) return false;
        if (MenuState::get().is_not(menu::State::Game)) return false;
        if (!is_user_host()) return false;

        if (showSeedInputBox &&
            KeyMap::get_key_code(menu::State::Game, InputName::Pause) ==
                event.keycode) {
            showSeedInputBox = false;
            return true;
        }

        if (!showSeedInputBox &&
            KeyMap::get_key_code(menu::State::Game, InputName::PlayerPickup) ==
                event.keycode) {
            showSeedInputBox = true;
            return true;
        }

        return ui_context.get()->process_keyevent(event);
    }

    virtual void onUpdate(float) override {
        auto map_ptr = GLOBALS.get_ptr<Map>(strings::globals::MAP);
        if (!map_ptr) return;

        if (!showSeedInputBox) {
            tempSeed = map_ptr->seed;
        }
    }

    void draw_seed_input(Map* map, float dt) {
        using namespace ui;
        begin(ui_context, dt);

        auto screen = Rectangle{0, 0, WIN_WF(), WIN_HF()};

        auto [_t, middle, _b] = rect::hsplit<3>(screen);
        auto [_l, popup, _r] = rect::vsplit<3>(middle);

        if (auto windowresult = window(Widget{popup, -10}); windowresult) {
            int z_index = windowresult.as<int>();
            auto [label, tf, buttons] = rect::hsplit<3>(popup, 20);

            text(Widget{label, windowresult.as<int>()},
                 // TODO translate
                 std::string("Enter Seed:"));

            if (auto result = textfield(
                    Widget{tf, z_index},
                    TextfieldData{
                        tempSeed,
                        [](const std::string& content) {
                            // TODO add max seed length
                            if (content.size() >= network::MAX_NAME_LENGTH)
                                return TextfieldValidationDecisionFlag::
                                    StopNewInput;
                            return TextfieldValidationDecisionFlag::Valid;
                        }});
                result) {
                tempSeed = (result.as<std::string>());
            }

            auto [_b1, randomize, _b2, select, _b3] =
                rect::vsplit<5>(buttons, 20);

            // TODO translate
            if (button(Widget{randomize, z_index}, "Randomize", true)) {
                const auto name = get_random_name_rot13();
                map->update_seed(name);
                showSeedInputBox = false;
            }

            // TODO translate
            if (button(Widget{select, z_index}, "Save Seed", true)) {
                map->update_seed(tempSeed);
                showSeedInputBox = false;
            }
        }

        end();
    }

    void draw_minimap(Map* map_ptr, float dt) {
        raylib::BeginMode3D((*cam).get());
        {
            raylib::rlTranslatef(-5, 0, 7.f);
            float scale = 0.10f;
            raylib::rlScalef(scale, scale, scale);
            raylib::DrawPlane((vec3){0.0f, -TILESIZE, 0.0f},
                              (vec2){40.0f, 40.0f}, DARKGRAY);
            map_ptr->onDraw(dt);
        }
        raylib::EndMode3D();
    }

    virtual void onDraw(float dt) override {
        TRACY_ZONE_SCOPED;
        if (!MenuState::s_in_game()) return;
        if (GameState::get().is(game::State::Paused)) return;

        auto map_ptr = GLOBALS.get_ptr<Map>(strings::globals::MAP);
        if (!map_ptr) return;

        // Only show minimap during lobby
        if (!map_ptr->showMinimap) return;

        draw_minimap(map_ptr, dt);

        if (is_user_host() && showSeedInputBox) draw_seed_input(map_ptr, dt);
    }
};