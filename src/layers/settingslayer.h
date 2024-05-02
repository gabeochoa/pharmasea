#pragma once

#include "../engine/layer.h"
#include "../engine/trie.h"

using namespace ui;

// TODO add support for customizing keymap

// TODO is there a way to automatically build this UI based on the
// settings::data format?
// (though some settings probably dont need to show)

struct SettingsLayer : public Layer {
    enum ActiveWindow {
        Root = 0,
        KeyBindings = 1,
    } activeWindow = ActiveWindow::Root;

    struct KeyBindingPopup {
        bool show = false;
        menu::State state = menu::State::UI;
        InputName input = InputName::Pause;
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

    float fullscreen_debounce = 0.f;
    float fullscreen_debounce_reset = 0.100f;

    SettingsLayer() : Layer("Settings") {
        ui_context = std::make_shared<ui::UIContext>();

        keyInputNames = magic_enum::enum_entries<InputName>();
        for (const auto& kv : keyInputNames) {
            keyBindingTrie.add(std::string(kv.second));
            keyInputValues.push_back(std::make_pair(
                kv.first,
                KeyMap::get_valid_inputs(menu::State::Game, kv.first)));
        }
    }

    virtual ~SettingsLayer() {
        // also write the keymap on exit
        Preload::get().write_keymap();
    }

    virtual bool onKeyPressed(KeyPressedEvent& event) override;
    virtual bool onGamepadButtonPressed(
        GamepadButtonPressedEvent& event) override;
    virtual void onUpdate(float dt) override;

    void save_and_exit();
    void exit_without_save();

    void draw_footer(Rectangle footer);
    void draw_base_screen(float);

    virtual void onDraw(float dt) override;

    void draw_column(Rectangle column, int index, Rectangle screen);
    void draw_keybinding_screen(float);
};
