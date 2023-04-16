
#pragma once

#include "../external_include.h"
#include "raylib.h"
//
#include "../engine.h"

struct AboutLayer : public Layer {
    std::shared_ptr<ui::UIContext> ui_context;

    // NOTE: this is not aligned on purpose
    const std::string about_info = R"(
A game by: 
    Gabe
    Brett
    Alice)";

    AboutLayer() : Layer("About") { ui_context.reset(new ui::UIContext()); }
    virtual ~AboutLayer() {}

    virtual void onEvent(Event& event) override {
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<KeyPressedEvent>(
            std::bind(&AboutLayer::onKeyPressed, this, std::placeholders::_1));
    }

    bool onKeyPressed(KeyPressedEvent& event) {
        if (MenuState::get().is_not(menu::State::About)) return false;
        if (event.keycode == raylib::KEY_ESCAPE) {
            MenuState::get().go_back();
            return true;
        }
        return ui_context.get()->process_keyevent(event);
    }

    void draw_ui(float dt) {
        raylib::SetExitKey(raylib::KEY_NULL);
        using namespace ui;

        ui_context->begin(dt);
        auto root = ui::components::mk_root();

        ui_context.get()->push_parent(root);
        {
            padding(*ui::components::mk_padding(Size_Px(100.f, 1.f),
                                                Size_FullH(0.f)));

            auto content = ui_context->own(Widget(
                {.mode = Children}, Size_Pct(1.f, 1.f), GrowFlags::Column));
            div(*content);

            ui_context.get()->push_parent(content);
            {
                padding(*ui::components::mk_padding(Size_Px(100.f, 1.f),
                                                    Size_Pct(0.5, 0.f)));
                auto about_text = ui_context->own(
                    Widget(Size_Px(200.f, 0.5f), Size_Px(400.f, 0.5f)));
                text(*about_text, about_info);
                if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID)),
                           "Back")) {
                    MenuState::get().go_back();
                }
                padding(*ui::components::mk_padding(Size_Px(50.f, 1.f),
                                                    Size_Pct(0.5, 0.f)));
            }
            ui_context.get()->pop_parent();
        }
        ui_context.get()->pop_parent();
        ui_context->end(root.get());
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
        ext::clear_background(ui_context->active_theme().background);
        draw_ui(dt);
    }
};
