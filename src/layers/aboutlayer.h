
#pragma once

#include "../external_include.h"
#include "raylib.h"
//
#include "../engine.h"
#include "../engine/ui/ui.h"

struct AboutLayer : public Layer {
    std::shared_ptr<ui::UIContext> ui_context;
    LayoutBox root_box;

    AboutLayer()
        : Layer(strings::menu::ABOUT),
          ui_context(std::make_shared<ui::UIContext>()),
          root_box(load_ui("resources/html/about.html", WIN_R())) {}

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

    void process_on_click(const std::string& id) {
        switch (hashString(id)) {
            case hashString(strings::i18n::BACK_BUTTON):
                MenuState::get().set(menu::State::Root);
                break;
        }
    }

    virtual void onDraw(float) override {
        if (MenuState::get().is_not(menu::State::About)) return;
        raylib::SetExitKey(raylib::KEY_NULL);
        ext::clear_background(ui_context->active_theme().background);

        elements::focus::begin();

        render_ui(ui_context, root_box, WIN_R(),
                  std::bind(&AboutLayer::process_on_click, *this,
                            std::placeholders::_1));

        elements::focus::end();
    }
};
