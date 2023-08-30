
#pragma once

#include "../engine/layer.h"
#include "../external_include.h"
#include "raylib.h"
//
#include "../engine.h"
#include "../engine/ui/ui.h"

struct AboutLayer : public Layer {
    std::shared_ptr<ui::UIContext> ui_context;

    AboutLayer()
        : Layer(strings::menu::ABOUT),
          ui_context(std::make_shared<ui::UIContext>()) {}
    virtual ~AboutLayer() {}

    bool onKeyPressed(KeyPressedEvent& event) override {
        if (MenuState::get().is_not(menu::State::About)) return false;
        if (event.keycode == raylib::KEY_ESCAPE) {
            MenuState::get().go_back();
            return true;
        }
        return ui_context.get()->process_keyevent(event);
    }

    virtual void onUpdate(float) override {
        // TODO can we just pass the "active menu state" to the layer
        // constructor and have app / layer handle this for us?
        //
        // Does pause not support this ^^ solution?
        if (MenuState::get().is_not(menu::State::About)) return;
        raylib::SetExitKey(raylib::KEY_NULL);
    }

    virtual void onDraw(float dt) override {
        if (MenuState::get().is_not(menu::State::About)) return;
        raylib::SetExitKey(raylib::KEY_NULL);
        ext::clear_background(ui::UI_THEME.background);
        using namespace ui;
        begin(ui_context, dt);

        auto window = Rectangle{0, 0, WIN_WF(), WIN_HF()};
        window = rect::lpad(window, 20);
        auto [info, back] = rect::hsplit<2>(window);
        back = rect::rpad(back, 50);
        back = rect::bpad(back, 25);

        text(Widget{info}, text_lookup(strings::ABOUT_INFO));
        if (button(Widget{back}, text_lookup(strings::i18n::BACK_BUTTON))) {
            MenuState::get().go_back();
        }

        end();
    }
};
