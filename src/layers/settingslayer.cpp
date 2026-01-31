
#include "settingslayer.h"

#include "../ah.h"
#include "../engine/input_utilities.h"
#include "../engine/settings.h"

void SettingsLayer::handleInput() {
    // Polling-based Alt+Enter fullscreen toggle (replaces onKeyPressed handler logic)
    // Check for Alt+Enter combo for fullscreen toggle
    bool alt_down = afterhours::input::is_key_down(raylib::KEY_LEFT_ALT) ||
                    afterhours::input::is_key_down(raylib::KEY_RIGHT_ALT);
    bool enter_pressed = afterhours::input::is_key_pressed(raylib::KEY_ENTER);
    bool alt_pressed = afterhours::input::is_key_pressed(raylib::KEY_LEFT_ALT) ||
                       afterhours::input::is_key_pressed(raylib::KEY_RIGHT_ALT);
    bool enter_down = afterhours::input::is_key_down(raylib::KEY_ENTER);

    bool fs_key_pressed = (enter_pressed && alt_down) || (alt_pressed && enter_down);

    if (fs_key_pressed && fullscreen_debounce <= 0) {
        Settings::get().toggle_fullscreen();
        fullscreen_debounce = fullscreen_debounce_reset;
    }

    if (MenuState::get().is_not(menu::State::Settings)) return;

    // Polling-based Escape key for navigation
    if (afterhours::input::is_key_pressed(raylib::KEY_ESCAPE)) {
        if (activeWindow != ActiveWindow::Root) {
            activeWindow = ActiveWindow::Root;
        } else {
            MenuState::get().go_back();
        }
        return;
    }

    // Polling-based gamepad MenuBack
    if (afterhours::input_ext::is_any_button_just_pressed(
            KeyMap::get_valid_inputs(menu::State::UI, InputName::MenuBack))) {
        exit_without_save();
        return;
    }
}

void SettingsLayer::onUpdate(float dt) {
    if (fullscreen_debounce > 0) fullscreen_debounce -= dt;

    handleInput();

    if (MenuState::get().is_not(menu::State::Settings)) return;
    raylib::SetExitKey(raylib::KEY_NULL);
}

void SettingsLayer::save_and_exit() {
    Preload::get().write_keymap();
    Settings::get().write_save_file();
    MenuState::get().go_back();
}

void SettingsLayer::exit_without_save() { MenuState::get().go_back(); }

void SettingsLayer::draw_footer(Rectangle footer) {
    if (activeWindow == ActiveWindow::KeyBindings)
        footer = rect::lpad(footer, 10);
    footer = rect::bpad(footer, 30);

    auto [force_save, back, reset_to_default, controls, input_type] =
        rect::vsplit<5>(rect::rpad(footer, 90), 20);

    if (button(Widget{force_save},
               TranslatableString(strings::i18n::EXIT_AND_SAVE), true)) {
        save_and_exit();
    }
    if (button(Widget{back}, TranslatableString(strings::i18n::EXIT_NO_SAVE),
               true)) {
        exit_without_save();
    }

    if (button(Widget{reset_to_default},
               TranslatableString(strings::i18n::RESET_ALL_SETTINGS), true)) {
        Settings::get().reset_to_default();
    }

    const auto controls_text = activeWindow == ActiveWindow::Root
                                   ? TranslatableString(strings::i18n::CONTROLS)
                                   : TranslatableString(strings::i18n::GENERAL);

    if (button(Widget{controls}, controls_text, true)) {
        switch (activeWindow) {
            default:
            case ActiveWindow::Root:
                activeWindow = ActiveWindow::KeyBindings;
                break;
            case ActiveWindow::KeyBindings:
                activeWindow = ActiveWindow::Root;
                break;
        }
    }

    if (activeWindow == ActiveWindow::Root) return;

    if (button(Widget{input_type},
               TranslatableString(selected_input_type == InputType::Gamepad
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

void SettingsLayer::draw_base_screen(float) {
    auto window = Rectangle{0, 0, WIN_WF(), WIN_HF()};
    auto content = rect::tpad(window, 20);
    content = rect::lpad(content, 10);

    auto [rows, footer] = rect::hsplit(content, 80);

    {
        auto [master_vol, music_vol, sfx_vol, ui_theme, resolution, language,
              streamer, postprocessing, lighting, snapCamera, vsync,
              fullscreen] = rect::hsplit<12>(rect::bpad(rows, 85), 20);

        {
            auto [label, control] = rect::vsplit(master_vol, 30);
            control = rect::rpad(control, 30);

            text(Widget{label},
                 TranslatableString(strings::i18n::MASTER_VOLUME));
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

            text(Widget{label},
                 TranslatableString(strings::i18n::MUSIC_VOLUME));
            if (auto result =
                    slider(Widget{control},
                           {.value = Settings::get().data.music_volume});
                result) {
                Settings::get().update_music_volume(result.as<float>());
            }
        }

        {
            auto [label, control] = rect::vsplit(sfx_vol, 30);
            control = rect::rpad(control, 30);

            text(Widget{label},
                 TranslatableString(strings::i18n::SOUND_VOLUME));
            if (auto result =
                    slider(Widget{control},
                           {.value = Settings::get().data.sound_volume});
                result) {
                Settings::get().update_sound_volume(result.as<float>());
            }
        }

        {
            auto [label, control] = rect::vsplit(ui_theme, 30);
            control = rect::rpad(control, 30);

            text(Widget{label}, TranslatableString(strings::i18n::THEME));
            if (auto result =
                    dropdown(Widget{control},
                             DropdownData{
                                 Settings::get().get_ui_theme_options(),
                                 Settings::get().get_ui_theme_selected_index(),
                             });
                result) {
                Settings::get().update_theme_from_index(result.as<int>());
            }
        }

        {
            auto [label, control] = rect::vsplit(resolution, 30);
            control = rect::rpad(control, 30);

            text(Widget{label}, TranslatableString(strings::i18n::RESOLUTION));
            if (auto result =
                    dropdown(Widget{control},
                             DropdownData{
                                 Settings::get().resolution_options(),
                                 Settings::get().get_current_resolution_index(),
                             });
                result) {
                Settings::get().update_resolution_from_index(result.as<int>());
            }
        }

        {
            auto [label, control] = rect::vsplit(language, 30);
            control = rect::rpad(control, 30);

            text(Widget{label}, TranslatableString(strings::i18n::LANGUAGE));
            if (auto result =
                    dropdown(Widget{control},
                             DropdownData{
                                 Settings::get().language_options(),
                                 Settings::get().get_current_language_index(),
                             });
                result) {
                Settings::get().update_language_from_index(result.as<int>());
            }
        }

        {
            auto [label, control] = rect::vsplit(streamer, 30);
            control = rect::rpad(control, 10);

            text(Widget{label},
                 TranslatableString(strings::i18n::SHOW_SAFE_BOX));

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

            text(Widget{label}, TranslatableString(strings::i18n::ENABLE_PPS));

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

        {
            auto [label, control] = rect::vsplit(lighting, 30);
            control = rect::rpad(control, 10);

            text(Widget{label},
                 TranslatableString(strings::i18n::ENABLE_LIGHTING));

            if (auto result = checkbox(
                    Widget{control},
                    CheckboxData{.selected =
                                     Settings::get().data.enable_lighting});
                result) {
                Settings::get().update_lighting_enabled(result.as<bool>());
            }
        }

        {
            auto [label, control] = rect::vsplit(snapCamera, 30);
            control = rect::rpad(control, 10);

            text(Widget{label}, TranslatableString(strings::i18n::SNAP_CAMERA));

            if (auto result = checkbox(
                    Widget{control},
                    CheckboxData{.selected =
                                     Settings::get().data.snapCameraTo90});
                result) {
                Settings::get().data.snapCameraTo90 = (result.as<bool>());
            }
        }

        {
            auto [label, control] = rect::vsplit(fullscreen, 30);
            control = rect::rpad(control, 10);

            text(Widget{label}, TranslatableString(strings::i18n::FULLSCREEN));

            if (auto result = checkbox(
                    Widget{control},
                    CheckboxData{.selected =
                                     Settings::get().data.isFullscreen});
                result) {
                Settings::get().update_fullscreen(result.as<bool>());
            }
        }

        {
            auto [label, control] = rect::vsplit(vsync, 30);
            control = rect::rpad(control, 10);

            text(Widget{label},
                 TranslatableString(strings::i18n::VSYNC_ENABLED));

            if (auto result = checkbox(
                    Widget{control},
                    CheckboxData{.selected =
                                     Settings::get().data.vsync_enabled});
                result) {
                Settings::get().data.vsync_enabled = (result.as<bool>());
                // Apply VSync setting immediately
                if (Settings::get().data.vsync_enabled) {
                    raylib::SetWindowState(raylib::FLAG_VSYNC_HINT);
                    log_info("VSync enabled via settings UI");
                } else {
                    raylib::ClearWindowState(raylib::FLAG_VSYNC_HINT);
                    log_info("VSync disabled via settings UI");
                }
            }
        }
    }

    // Footer
    draw_footer(footer);
}

void SettingsLayer::onDraw(float dt) {
    if (MenuState::get().is_not(menu::State::Settings)) return;
    ext::clear_background(ui::UI_THEME.background);

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

void SettingsLayer::draw_column(Rectangle column, int index, Rectangle screen) {
    // NOTE: we only draw the first N, each state only has N inputs
    // today but if you want all then use this

    constexpr int num_inputs = magic_enum::enum_count<InputName>();
    constexpr int num_per_col = 12;  // (int) (num_inputs / 2.f);

    auto key_rects_1 = rect::hsplit<num_per_col>(column, 15);

    const auto _get_label_for_key =
        [](menu::State state,
           InputName name) -> tl::expected<std::string, std::string> {
        const auto key = afterhours::input_ext::get_first_key(
            KeyMap::get_valid_inputs(state, name));
        if (!key.has_value()) return tl::unexpected("input not used in this state");
        return afterhours::input_ext::name_for_input(key.value());
    };

    const auto _get_label_for_gamepad =
        [](menu::State state,
           InputName name) -> tl::expected<std::string, std::string> {
        const auto button = afterhours::input_ext::get_first_button(
            KeyMap::get_valid_inputs(state, name));
        if (!button.has_value())
            return tl::unexpected("input not used in this state");
        return afterhours::input_ext::name_for_input(button.value());
    };
    const auto _get_label =
        [=, *this](menu::State state,
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
        return tl::unexpected("input not used in this state");
    };

    const auto _get_icon_for_key =
        [](menu::State state,
           InputName name) -> tl::expected<std::string, std::string> {
        const auto key = afterhours::input_ext::get_first_key(
            KeyMap::get_valid_inputs(state, name));
        if (!key.has_value()) return tl::unexpected("input not used in this state");
        auto icon = afterhours::input_ext::icon_for_input(key.value());
        if (icon.empty()) return tl::unexpected("icon not found");
        return icon;
    };

    const auto _get_icon_for_gamepad =
        [](menu::State state,
           InputName name) -> tl::expected<std::string, std::string> {
        const auto button = afterhours::input_ext::get_first_button(
            KeyMap::get_valid_inputs(state, name));
        if (!button.has_value())
            return tl::unexpected("input not used in this state");
        return afterhours::input_ext::icon_for_input(button.value());
    };

    const auto _get_icon =
        // TODO
        [=, *this](menu::State state,
                   InputName name) -> tl::expected<std::string, std::string> {
        switch (selected_input_type) {
            case Keyboard:
                return _get_icon_for_key(state, name);
            case Gamepad:
                return _get_icon_for_gamepad(state, name);
            case GamepadWithAxis:
                // TODO
                return tl::unexpected("input not used in this state");
        }
    };

    const auto _keys_for_state = [=, *this](
                                     menu::State state,
                                     std::array<Rectangle, num_per_col> rects,
                                     int starting_index = 0) {
        int rendering_index = 0;
        for (int i = starting_index; i < num_inputs; i++) {
            const auto kv = keyInputNames[i];

            bool has_icon = false;
            auto checkbox_content = _get_icon(state, kv.first);
            if (!checkbox_content.has_value()) {
                checkbox_content = _get_label(state, kv.first);
                if (!checkbox_content.has_value()) {
                    log_trace("{} => {}", kv.second, checkbox_content.error());
                    continue;
                }
            } else {
                has_icon = true;
            }

            // TODO we have more inptus than items in col
            // if we go
            if (rendering_index >= num_per_col) {
                continue;
            }

            auto [label, remap_button] =
                rect::vsplit<2>(rects[rendering_index], 10);

            rendering_index++;

            text(Widget{label},
                 TODO_TRANSLATE(util::space_between_caps(kv.second),
                                TodoReason::KeyName));

            if (auto result =
                    checkbox(Widget{remap_button},
                             CheckboxData{.content = checkbox_content.value(),
                                          .content_is_icon = has_icon});
                result) {
                // TODO disabling popup for now
                //
                /*
                key_binding_popup =
                    KeyBindingPopup{.show = result.as<bool>(),
                                    .state = state,
                                    .input = kv.first};
                                    */
            }
        }
    };

    if (index == 0) {
        _keys_for_state(menu::State::UI, key_rects_1);
    } else if (index == 1) {
        _keys_for_state(menu::State::Game, key_rects_1, 1);
    }

    // TODO pressing UI keys when this popup is open still uses them

    if (key_binding_popup.show) {
        auto [_t, middle, _b] = rect::hsplit<3>(screen);
        auto [_l, popup, _r] = rect::vsplit<3>(middle);
        if (auto windowresult = window(Widget{popup, -10}); windowresult) {
            auto [label, input] = rect::hsplit<2>(popup);

            text(Widget{label, windowresult.as<int>()},
                 TODO_TRANSLATE(util::space_between_caps(magic_enum::enum_name(
                                    key_binding_popup.input)),
                                TodoReason::KeyName));

            bool has_icon = false;
            auto input_descr =
                _get_icon(key_binding_popup.state, key_binding_popup.input);
            if (!input_descr.has_value()) {
                input_descr = _get_label(key_binding_popup.state,
                                         key_binding_popup.input);
            } else {
                has_icon = true;
            }

            if (input_descr.has_value()) {
                if (auto control_result = control_input_field(
                        Widget{input, windowresult.as<int>()},
                        TextfieldData{.content = input_descr.value(),
                                      .content_is_icon = has_icon});
                    control_result) {
                    KeyMap::get().set_mapping(key_binding_popup.state,
                                              key_binding_popup.input,
                                              control_result.as<AnyInput>());
                }
            }
        }
    }
}

void SettingsLayer::draw_keybinding_screen(float) {
    auto screen = Rectangle{0, 0, WIN_WF(), WIN_HF()};
    auto content = rect::tpad(screen, 20);

    auto [body, footer] = rect::hsplit(content, 80);

    {
        body = rect::bpad(body, 95);
        constexpr int num_cols = 2;
        auto columns = rect::vsplit<num_cols>(body);

        for (size_t i = 0; i < columns.size(); i++) {
            columns[i] = rect::hpad(columns[i], 10);
            draw_column(columns[i], (int) i, screen);
        }
    }

    // Footer
    draw_footer(footer);
}
