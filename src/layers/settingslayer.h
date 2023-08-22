#pragma once

#include "../engine.h"
#include "../engine/app.h"
#include "../engine/settings.h"
#include "../engine/ui.h"
#include "../external_include.h"
#include "../preload.h"
#include "raylib.h"

using namespace ui;

// TODO add support for customizing keymap

// TODO is there a way to automatically build this UI based on the
// settings::data format?
// (though some settings probably dont need to show)

struct SettingsLayer : public Layer {
    // TODO add way to get into keybindings mode
    enum ActiveWindow {
        Root = 0,
        KeyBindings = 1,
    } activeWindow = ActiveWindow::KeyBindings;

    struct KeyBindingPopup {
        bool show = false;
        menu::State state;
        InputName input;
    } key_binding_popup;

    InputType selected_input_type = InputType::Keyboard;

    Trie keyBindingTrie;
    std::array<std::pair<InputName, std::string_view>,
               magic_enum::enum_count<InputName>()>
        keyInputNames;
    std::vector<std::pair<InputName, AnyInputs>> keyInputValues;

    std::shared_ptr<ui::UIContext> ui_context;
    bool windowSizeDropdownState = false;
    int windowSizeDropdownIndex = 0;
    bool resolution_dropdown_open = false;
    int resolution_selected_index = 0;

    bool language_dropdown_open = false;
    int language_selected_index = 0;

    SettingsLayer() : Layer("Settings") {
        ui_context = std::make_shared<ui::UIContext>();

        resolution_selected_index =
            Settings::get().get_current_resolution_index();
        if (resolution_selected_index < 0) resolution_selected_index = 0;

        language_selected_index = Settings::get().get_current_language_index();
        if (language_selected_index < 0) language_selected_index = 0;

        keyInputNames = magic_enum::enum_entries<InputName>();
        for (const auto& kv : keyInputNames) {
            keyBindingTrie.add(std::string(kv.second));
            keyInputValues.push_back(std::make_pair(
                kv.first,
                KeyMap::get_valid_inputs(menu::State::Game, kv.first)));
        }
    }

    virtual ~SettingsLayer() {}

    virtual bool onKeyPressed(KeyPressedEvent& event) override {
        if (MenuState::get().is_not(menu::State::Settings)) return false;

        //
        if (event.keycode == raylib::KEY_ESCAPE &&
            activeWindow != ActiveWindow::Root) {
            activeWindow = ActiveWindow::Root;
            return true;
        }

        if (event.keycode == raylib::KEY_ESCAPE) {
            MenuState::get().go_back();
            return true;
        }
        return ui_context.get()->process_keyevent(event);
    }

    virtual bool onGamepadButtonPressed(
        GamepadButtonPressedEvent& event) override {
        if (MenuState::get().is_not(menu::State::Settings)) return false;
        return ui_context.get()->process_gamepad_button_event(event);
    }

    virtual void onUpdate(float) override {
        if (MenuState::get().is_not(menu::State::Settings)) return;
        raylib::SetExitKey(raylib::KEY_NULL);
    }

    void exit_layer() {
        // TODO when you hit escape also need to call exit_layer
        Preload::get().write_keymap();
        MenuState::get().go_back();
    }

    void draw_base_screen(float dt) {
        auto window = Rectangle{0, 0, WIN_WF(), WIN_HF()};
        auto content = rect::tpad(window, 20);
        content = rect::lpad(content, 10);

        auto [rows, footer] = rect::hsplit(content, 80);
        footer = rect::bpad(footer, 50);

        {
            auto [master_vol, music_vol, resolution, language, streamer,
                  postprocessing] = rect::hsplit<6>(rect::bpad(rows, 85), 20);

            {
                auto [label, control] = rect::vsplit(master_vol, 30);
                control = rect::rpad(control, 30);

                text(Widget{label}, text_lookup(strings::i18n::MASTER_VOLUME));
                if (auto result =
                        slider(Widget{control},
                               {.value = Settings::get().data.master_volume});
                    result) {
                    Settings::get().update_master_volume(result.as<float>());
                }
            }

            {
                auto [label, control] = rect::vsplit(music_vol, 30);
                control = rect::rpad(control, 30);

                text(Widget{label}, text_lookup(strings::i18n::MUSIC_VOLUME));
                if (auto result =
                        slider(Widget{control},
                               {.value = Settings::get().data.music_volume});
                    result) {
                    Settings::get().update_music_volume(result.as<float>());
                }
            }

            {
                auto [label, control] = rect::vsplit(resolution, 30);
                control = rect::rpad(control, 30);

                text(Widget{label}, text_lookup(strings::i18n::RESOLUTION));
                if (auto result = dropdown(
                        Widget{control},
                        DropdownData{
                            Settings::get().resolution_options(),
                            Settings::get().get_current_resolution_index(),
                        });
                    result) {
                    Settings::get().update_resolution_from_index(
                        result.as<int>());
                }
            }

            {
                auto [label, control] = rect::vsplit(language, 30);
                control = rect::rpad(control, 30);

                text(Widget{label}, text_lookup(strings::i18n::LANGUAGE));
                if (auto result = dropdown(
                        Widget{control},
                        DropdownData{
                            Settings::get().language_options(),
                            Settings::get().get_current_language_index(),
                        });
                    result) {
                    Settings::get().update_language_from_index(
                        result.as<int>());
                }
            }

            {
                auto [label, control] = rect::vsplit(streamer, 30);
                control = rect::rpad(control, 10);

                text(Widget{label}, text_lookup(strings::i18n::SHOW_SAFE_BOX));

                // TODO default value wont be setup correctly without this
                // bool sssb = Settings::get().data.show_streamer_safe_box;
                if (auto result = checkbox(
                        Widget{control},
                        CheckboxData{
                            .selected =
                                Settings::get().data.show_streamer_safe_box});
                    result) {
                    Settings::get().update_streamer_safe_box(result.as<bool>());
                }
            }

            {
                auto [label, control] = rect::vsplit(postprocessing, 30);
                control = rect::rpad(control, 10);

                text(Widget{label}, text_lookup(strings::i18n::ENABLE_PPS));

                // TODO default value wont be setup correctly without this
                // bool sssb = Settings::get().data.enable_postprocessing;
                if (auto result = checkbox(
                        Widget{control},
                        CheckboxData{
                            .selected =
                                Settings::get().data.enable_postprocessing});
                    result) {
                    Settings::get().update_post_processing_enabled(
                        result.as<bool>());
                }
            }
        }

        // Footer
        {
            auto [back, controls, _] =
                rect::vsplit<3>(rect::rpad(footer, 30), 20);
            if (button(Widget{back}, text_lookup(strings::i18n::BACK_BUTTON),
                       true)) {
                exit_layer();
            }
            // TODO add string Edit Controls
            if (button(Widget{controls}, text_lookup(strings::i18n::CONTROLS),
                       true)) {
                activeWindow = ActiveWindow::KeyBindings;
            }
        }
    }

    virtual void onDraw(float dt) override {
        if (MenuState::get().is_not(menu::State::Settings)) return;
        ext::clear_background(ui_context->active_theme().background);

        begin(ui_context, dt);

        switch (activeWindow) {
            default:
            case ActiveWindow::Root:
                draw_base_screen(dt);
                break;
            case ActiveWindow::KeyBindings:
                draw_keybinding_screen(dt);
                break;
        }

        end();
    }

    void draw_keybinding_screen(float) {
        auto screen = Rectangle{0, 0, WIN_WF(), WIN_HF()};
        auto content = rect::tpad(screen, 20);

        auto [body, footer] = rect::hsplit(content, 80);

        {
            body = rect::bpad(body, 95);
            auto [col1, col2] = rect::vsplit<2>(body);

            // These should match
            col1 = rect::lpad(col1, 20);
            col1 = rect::rpad(col1, 70);
            col2 = rect::lpad(col2, 10);
            col2 = rect::rpad(col2, 80);

            // NOTE: we only draw the first N, each state only has N inputs
            // today but if you want all then use this

            constexpr int num_inputs = magic_enum::enum_count<InputName>();
            constexpr int num_per_col = 12;  // (int) (num_inputs / 2.f);

            // we want both cols to look the same so they should always match
            auto key_rects_1 = rect::hsplit<num_per_col>(col1, 15);
            auto key_rects_2 = rect::hsplit<num_per_col>(col2, 15);

            const auto _get_label_for_key =
                [](menu::State state,
                   InputName name) -> tl::expected<std::string, std::string> {
                const auto keys = KeyMap::get_valid_keys(state, name);
                if (keys.empty())
                    return tl::unexpected("input not used in this state");
                return KeyMap::get().name_for_input(keys[0]);
            };

            const auto _get_label_for_gamepad =
                [](menu::State state,
                   InputName name) -> tl::expected<std::string, std::string> {
                const auto button = KeyMap::get_button(state, name);
                if (button == raylib::GAMEPAD_BUTTON_UNKNOWN)
                    return tl::unexpected("input not used in this state");
                return KeyMap::get().name_for_input(button);
            };

            const auto _get_label =
                [=](menu::State state,
                    InputName name) -> tl::expected<std::string, std::string> {
                switch (selected_input_type) {
                    case Keyboard:
                        return _get_label_for_key(state, name);
                    case Gamepad:
                        return _get_label_for_gamepad(state, name);
                    case GamepadWithAxis:
                        // TODO
                        log_error("idk how to handle axis right now");
                        break;
                }
            };

            const auto _keys_for_state =
                [=](menu::State state, std::array<Rectangle, num_per_col> rects,
                    int starting_index = 0) {
                    int rendering_index = 0;
                    for (int i = starting_index; i < num_inputs; i++) {
                        const auto kv = keyInputNames[i];
                        const auto i_label = _get_label(state, kv.first);
                        if (!i_label.has_value()) {
                            log_trace("{} => {}", kv.second, i_label.error());
                            continue;
                        }

                        auto [label, remap_button] =
                            rect::vsplit<2>(rects[rendering_index++], 20);

                        text(Widget{label},
                             util::space_between_caps(kv.second));

                        if (auto result = checkbox(Widget{remap_button}, CheckboxData{.content=i_label.value()}); result) {
                        key_binding_popup = KeyBindingPopup {
                            .show = result.as<bool>(),
                            .state = state,
                            .input = kv.first
                        };

                        }

                    }
                };

            _keys_for_state(menu::State::UI, key_rects_1);
            _keys_for_state(menu::State::Game, key_rects_2, 1);

            if(key_binding_popup.show){
                auto [_t, middle, _b] = rect::hsplit<3>(screen);
                auto [_l, popup, _r] = rect::vsplit<3>(middle);
                if(auto windowresult = window(Widget{popup, -10}); windowresult){
                    std::cout << "popup render" << std::endl;
                }
            }
        }

        // Footer
        {
            footer = rect::lpad(footer, 10);
            footer = rect::bpad(footer, 50);

            auto [back, controls, input_type] =
                rect::vsplit<3>(rect::rpad(footer, 30), 20);
            if (button(Widget{back}, text_lookup(strings::i18n::BACK_BUTTON),
                       true)) {
                exit_layer();
            }
            // TODO add string Edit Controls
            if (button(Widget{controls}, text_lookup(strings::i18n::GENERAL),
                       true)) {
                activeWindow = ActiveWindow::Root;
            }

            if (button(Widget{input_type},
                       text_lookup(selected_input_type == InputType::Gamepad
                                       ? strings::i18n::GAMEPAD
                                       : strings::i18n::KEYBOARD),
                       true)) {
                switch (selected_input_type) {
                    case Keyboard:
                        selected_input_type = InputType::Gamepad;
                        break;
                    case Gamepad:
                        selected_input_type = InputType::Keyboard;
                        break;
                    case GamepadWithAxis:
                        break;
                }
            }
        }
    }
};
