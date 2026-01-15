#pragma once

#include "../engine/constexpr_containers.h"
#include "../engine/runtime_globals.h"
#include "base_game_renderer.h"

constexpr size_t CHOOSABLE_STATES = (magic_enum::enum_count<game::State>()  //
                                     - 1  // InMenu
);

constexpr CEMap<int, game::State, CHOOSABLE_STATES> choosable_game_states = {{{
    {2, game::State::InGame},
    {7, game::State::ModelTest},
    {1, game::State::Lobby},
}}};

struct DebugSettingsLayer : public BaseGameRendererLayer {
    bool should_show_overlay = false;
    bool debug_ui_enabled = false;
    bool no_clip_enabled = false;
    bool skip_ingredient_match = false;

    DebugSettingsLayer() : BaseGameRendererLayer("DebugSettings") {
        sync_globals();
    }

    void sync_globals() {
        globals::set_debug_ui_enabled(debug_ui_enabled);
        globals::set_no_clip_enabled(no_clip_enabled);
        globals::set_skip_ingredient_match(skip_ingredient_match);
    }

    bool onGamepadButtonPressed(GamepadButtonPressedEvent& event) override {
        if (KeyMap::get_button(menu::State::Game, InputName::ToggleDebug) ==
            event.button) {
            debug_ui_enabled = !debug_ui_enabled;
            sync_globals();
            return true;
        }
        if (KeyMap::get_button(menu::State::Game,
                               InputName::ToggleNetworkView) == event.button) {
            no_clip_enabled = !no_clip_enabled;
            sync_globals();
            return true;
        }

        if (KeyMap::get_button(menu::State::Game,
                               InputName::SkipIngredientMatch) ==
            event.button) {
            skip_ingredient_match = !skip_ingredient_match;
            sync_globals();
            return true;
        }

        if (!baseShouldRender()) return false;

        if (KeyMap::get_button(menu::State::Game, InputName::Pause) ==
            event.button) {
            GameState::get().go_back();
            return true;
        }

        return ui_context->process_gamepad_button_event(event);
    }

    bool onKeyPressed(KeyPressedEvent& event) override {
        if (should_show_overlay &&
            KeyMap::get_key_code(menu::State::Game, InputName::Pause) ==
                event.keycode) {
            should_show_overlay = false;
            return true;
        }

        if (KeyMap::get_key_code(menu::State::Game,
                                 InputName::ToggleDebugSettings) ==
            event.keycode) {
            should_show_overlay = !should_show_overlay;
            return true;
        }

        // TODO can we catch if you are using get_button in onKeyPressed and
        // warn?

        if (KeyMap::get_key_code(menu::State::Game,
                                 InputName::SkipIngredientMatch) ==
            event.keycode) {
            skip_ingredient_match = !skip_ingredient_match;
            sync_globals();
            return true;
        }

        if (KeyMap::get_key_code(menu::State::Game, InputName::ToggleDebug) ==
            event.keycode) {
            debug_ui_enabled = !debug_ui_enabled;
            sync_globals();
            return true;
        }
        if (KeyMap::get_key_code(menu::State::Game,
                                 InputName::ToggleNetworkView) ==
            event.keycode) {
            no_clip_enabled = !no_clip_enabled;
            sync_globals();
            return true;
        }
        if (!baseShouldRender()) return false;

        return ui_context->process_keyevent(event);
    }

    virtual ~DebugSettingsLayer() {}

    virtual bool shouldSkipRender() override { return !shouldRender(); }
    bool shouldRender() { return should_show_overlay; }

    virtual void onUpdate(float) override {}

    void draw_game_state_controls(Rectangle container) {
        using namespace ui;

        std::vector<std::string> val;
        {
            for (const auto& kv : choosable_game_states) {
                val.push_back(
                    std::string(magic_enum::enum_name<game::State>(kv.second)));
            }
        }

        const auto get_selected_game_state_index = []() -> int {
            int state_value = static_cast<int>(
                magic_enum::enum_integer<game::State>(GameState::get().read()));

            int i = 0;
            for (const auto& kv : choosable_game_states) {
                if (kv.first == state_value) return i;
                i++;
            }
            return 0;
        };

        const auto update_game_state_option = [](int index) -> void {
            GameState::get().set(choosable_game_states.data[index].second);
        };

        auto [game_state_label, game_state_dropdown, _a] =
            rect::vsplit<3>(container, 10);

        text(Widget{game_state_label}, NO_TRANSLATE("Game State"));
        if (auto result = dropdown(Widget{game_state_dropdown},
                                   DropdownData{
                                       val,
                                       get_selected_game_state_index(),
                                   });
            result) {
            // skip going to lobby as it doesnt work anyway
            if (result.as<int>() != 3) {
                update_game_state_option(result.as<int>());
            }
        }
    }

    virtual void onDrawUI(float) override {
        if (!no_clip_enabled) {
            DrawTextEx(Preload::get().font, "Press = to enable no-clip",
                       vec2{200, 90}, 20, 0, BLUE);
        } else {
            DrawTextEx(Preload::get().font, "NO CLIP ENABLED", vec2{200, 90},
                       20, 0, RED);
        }

        if (!debug_ui_enabled) {
            DrawTextEx(Preload::get().font, "Press \\ to toggle debug UI",
                       vec2{200, 70}, 20, 0, RED);
            return;
        }

        using namespace ui;
        auto window = Rectangle{0, 0, WIN_WF(), WIN_HF()};
        window = rect::bpad(window, 50);

        auto content = rect::lpad(window, 10);
        content = rect::rpad(content, 90);

        const auto [_a, debug_mode, no_clip, skip_ingredient_check, game_state,
                    enabled_drinks, _b] = rect::hsplit<7>(content, 10);

        {
            auto [label, control] = rect::vsplit(debug_mode, 30);
            control = rect::rpad(control, 10);

            text(Widget{label}, NO_TRANSLATE("Debug Mode"));

            if (auto result =
                    checkbox(Widget{control},
                             CheckboxData{.selected = debug_ui_enabled});
                result) {
                debug_ui_enabled = result.as<bool>();
                sync_globals();
            }
        }

        {
            auto [label, control] = rect::vsplit(no_clip, 30);
            control = rect::rpad(control, 10);

            text(Widget{label}, NO_TRANSLATE("No Clip"));

            if (auto result = checkbox(
                    Widget{control}, CheckboxData{.selected = no_clip_enabled});
                result) {
                no_clip_enabled = result.as<bool>();
                sync_globals();
            }
        }

        {
            auto [label, control] = rect::vsplit(skip_ingredient_check, 30);
            control = rect::rpad(control, 10);

            text(Widget{label}, NO_TRANSLATE("Skip Ingredient Check"));

            if (auto result =
                    checkbox(Widget{control},
                             CheckboxData{.selected = skip_ingredient_match});
                result) {
                skip_ingredient_match = result.as<bool>();
                sync_globals();
            }
        }

        draw_game_state_controls(game_state);
    }
};
