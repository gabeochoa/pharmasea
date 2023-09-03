#pragma once

#include "../drawing_util.h"
#include "../engine/event.h"
#include "../external_include.h"
//
#include "../engine.h"
#include "../globals.h"
#include "../map.h"
#include "../preload.h"
//
#include "../camera.h"
#include "../components/can_hold_furniture.h"
#include "../components/can_hold_item.h"
#include "../components/collects_user_input.h"
#include "../components/is_progression_manager.h"
#include "../components/is_spawner.h"
#include "../engine/layer.h"
#include "../engine/statemanager.h"
#include "../entity_helper.h"
#include "raylib.h"

struct GameDebugLayer : public Layer {
    bool debug_ui_enabled = false;
    bool no_clip_enabled = false;
    std::shared_ptr<ui::UIContext> ui_context;

    GameDebugLayer() : Layer(strings::menu::GAME) {
        GLOBALS.set("debug_ui_enabled", &debug_ui_enabled);
        GLOBALS.set("no_clip_enabled", &no_clip_enabled);
        ui_context = std::make_shared<ui::UIContext>();
    }
    virtual ~GameDebugLayer() {}

    void toggle_to_planning() {
        GameState::get().toggle_to_planning();
        EntityHelper::invalidatePathCache();
    }

    void toggle_to_inround() {
        GameState::get().toggle_to_inround();
        EntityHelper::invalidatePathCache();
    }

    bool onGamepadAxisMoved(GamepadAxisMovedEvent&) override { return false; }

    bool onGamepadButtonPressed(GamepadButtonPressedEvent& event) override {
        if (!MenuState::s_in_game()) return false;
        if (KeyMap::get_button(menu::State::Game,
                               InputName::ToggleToPlanning) == event.button) {
            toggle_to_planning();
            return true;
        }
        if (KeyMap::get_button(menu::State::Game, InputName::ToggleToInRound) ==
            event.button) {
            toggle_to_inround();
            return true;
        }
        if (KeyMap::get_button(menu::State::Game, InputName::ToggleDebug) ==
            event.button) {
            debug_ui_enabled = !debug_ui_enabled;
            return true;
        }
        if (KeyMap::get_button(menu::State::Game,
                               InputName::ToggleNetworkView) == event.button) {
            no_clip_enabled = !no_clip_enabled;
            return true;
        }
        return ui_context.get()->process_gamepad_button_event(event);
    }

    bool onKeyPressed(KeyPressedEvent& event) override {
        if (!MenuState::s_in_game()) return false;
        if (KeyMap::get_key_code(menu::State::Game,
                                 InputName::ToggleToPlanning) ==
            event.keycode) {
            toggle_to_planning();
            return true;
        }
        if (KeyMap::get_key_code(menu::State::Game,
                                 InputName::ToggleToInRound) == event.keycode) {
            toggle_to_inround();
            return true;
        }
        if (KeyMap::get_key_code(menu::State::Game, InputName::ToggleLobby) ==
            event.keycode) {
            EntityHelper::invalidatePathCache();
            GameState::get().set(game::State::Lobby);
            return true;
        }
        if (KeyMap::get_key_code(menu::State::Game, InputName::ToggleDebug) ==
            event.keycode) {
            debug_ui_enabled = !debug_ui_enabled;
            return true;
        }
        if (KeyMap::get_key_code(menu::State::Game,
                                 InputName::ToggleNetworkView) ==
            event.keycode) {
            no_clip_enabled = !no_clip_enabled;
            return true;
        }
        return ui_context.get()->process_keyevent(event);
    }

    virtual void onUpdate(float) override {
        if (!MenuState::s_in_game()) return;
    }

    virtual void onDraw(float dt) override {
        if (!MenuState::s_in_game()) return;
        if (GameState::get().is(game::State::Paused)) return;
        if (!debug_ui_enabled) {
            DrawTextEx(Preload::get().font, "Press \\ to toggle debug UI",
                       vec2{200, 70}, 20, 0, RED);
            return;
        }

        draw_debug_ui(dt);
    }

    void draw_debug_ui(float dt) {
        auto map_ptr = GLOBALS.get_ptr<Map>(strings::globals::MAP);
        using namespace ui;
        begin(ui_context, dt);

        auto window = Rectangle{0, 0, WIN_WF(), WIN_HF()};
        auto [l_col, _] = rect::vsplit(window, 20);

        auto [_ping_game_version, menu_state, player_info, round_info,
              _bottom_padding] = rect::hsplit<5>(l_col, 20);

        {
            auto [menu_div, game_div] = rect::hsplit<2>(menu_state, 10);
            text(Widget{menu_div}, std::string(MenuState::get().tostring()));
            text(Widget{game_div}, std::string(GameState::get().tostring()));
        }

        // Player Info
        {
            if (map_ptr) {
                OptEntity player = map_ptr->get_remote_with_cui();
                if (player) {
                    auto [id_div, position_div, holding_div, item_div] =
                        rect::hsplit<4>(player_info, 10);

                    text(Widget{id_div}, fmt::format("{}", (player->id)));
                    text(Widget{position_div},
                         fmt::format("{}", (player->get<Transform>().pos())));
                    text(Widget{holding_div},
                         fmt::format("holding furniture?: {}",
                                     player->get<CanHoldFurniture>()
                                             .is_holding_furniture()
                                         ? player->get<CanHoldFurniture>()
                                               .furniture()
                                               ->get<DebugName>()
                                               .name()
                                         : "Empty"));
                    text(
                        Widget{item_div},
                        fmt::format("holding item?: {}",
                                    player->get<CanHoldItem>().is_holding_item()
                                        ? player->get<CanHoldItem>()
                                              .item()
                                              ->get<DebugName>()
                                              .name()
                                        : "Empty"));
                } else {
                    text(Widget{player_info}, "No matching player found");
                }
            } else {
                text(Widget{player_info}, "Map not valid");
            }
        }

        // Round Info
        {
            if (map_ptr) {
                OptEntity sophie =
                    EntityHelper::getFirstWithComponent<IsProgressionManager>();
                if (sophie) {
                    auto [round_time_div, round_spawn_div, drinks_div,
                          ingredients_div] = rect::hsplit<4>(round_info, 10);

                    text(Widget{round_time_div},
                         fmt::format("Round Length: {}",
                                     round_settings::ROUND_LENGTH_S));

                    {
                        auto [num_spawned, countdown] =
                            rect::hsplit<2>(round_spawn_div);

                        auto spawners = EntityHelper::getAllWithType(
                            EntityType::CustomerSpawner);

                        Entity& spawner = spawners[0];
                        const IsSpawner& iss = spawner.get<IsSpawner>();
                        text(Widget{num_spawned},
                             fmt::format("Num Spawned: {} (max? {})",
                                         iss.get_num_spawned(), iss.hit_max()));
                    }

                } else {
                    text(Widget{round_info}, "No matching sophie found");
                }
            } else {
                text(Widget{round_info}, "Map not valid");
            }
        }

        end();
    }
};
