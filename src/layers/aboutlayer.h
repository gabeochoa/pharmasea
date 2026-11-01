
#pragma once

#include "../engine/input_utilities.h"
#include "../engine/layer.h"
#include "../external_include.h"
#include "raylib.h"
//
#include "../engine.h"
#include "../engine/svg_renderer.h"
#include "../engine/ui/ui.h"

struct AboutLayer : public Layer {
    SVGRenderer svg;
    std::shared_ptr<ui::UIContext> ui_context;

    AboutLayer()
        : Layer(strings::menu::ABOUT),
          svg(SVGRenderer("about_screen")),
          ui_context(std::make_shared<ui::UIContext>()) {}
    virtual ~AboutLayer() {}

    void handleInput() {
        if (MenuState::get().is_not(menu::State::About)) return;

        // Polling-based back navigation (replaces
        // onKeyPressed/onGamepadButtonPressed handlers)
        if (afterhours::input::is_key_pressed(raylib::KEY_ESCAPE)) {
            MenuState::get().go_back();
            return;
        }
        if (afterhours::input_ext::is_any_button_just_pressed(
                KeyMap::get_valid_inputs(menu::State::UI,
                                         InputName::MenuBack))) {
            MenuState::get().go_back();
            return;
        }
    }

    virtual void onUpdate(float) override {
        // TODO can we just pass the "active menu state" to the layer
        // constructor and have app / layer handle this for us?
        //
        // Does pause not support this ^^ solution?
        if (MenuState::get().is_not(menu::State::About)) return;
        raylib::SetExitKey(raylib::KEY_NULL);

        handleInput();
    }

    virtual void onDraw(float dt) override {
        if (MenuState::get().is_not(menu::State::About)) return;
        raylib::SetExitKey(raylib::KEY_NULL);
        ext::clear_background(ui::UI_THEME.background);
        using namespace ui;
        begin(ui_context, dt);

        svg.draw_background();
        svg.text("AboutText", NO_TRANSLATE(strings::ABOUT_INFO));

        if (svg.button("BackButton",
                       TranslatableString(strings::i18n::BACK_BUTTON))) {
            MenuState::get().go_back();
        }

        end();
    }
};
