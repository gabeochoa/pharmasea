
#pragma once

#include "app.h"
#include "external_include.h"
#include "layer.h"
#include "menu.h"
#include "raylib.h"
#include "ui.h"
#include "ui_autolayout.h"
#include "ui_theme.h"
#include "ui_widget.h"
#include "uuid.h"

struct AboutLayer : public Layer {
    std::shared_ptr<ui::UIContext> ui_context;

    AboutLayer() : Layer("About") {
        minimized = false;

        ui_context.reset(new ui::UIContext());

        ui_context.get()->init();
        ui_context.get()->set_font(Preload::get().font);
        // TODO we should probably enforce that you cant do this
        // and we should have ->set_base_theme()
        // and push_theme separately, if you end() with any stack not empty...
        // thats a flag
        ui_context->push_theme(ui::DEFAULT_THEME);
    }
    virtual ~AboutLayer() {}
    virtual void onAttach() override {}
    virtual void onDetach() override {}

    virtual void onEvent(Event& event) override {
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<KeyPressedEvent>(
            std::bind(&AboutLayer::onKeyPressed, this, std::placeholders::_1));
    }

    bool onKeyPressed(KeyPressedEvent& event) {
        if (Menu::get().is_not(Menu::State::About)) return false;
        if (event.keycode == KEY_ESCAPE) {
            Menu::get().go_back();
            return true;
        }
        return ui_context.get()->process_keyevent(event);
    }

    void draw_ui(float dt) {
        SetExitKey(KEY_ESCAPE);
        using namespace ui;

        // TODO move to input
        bool mouseDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        vec2 mousepos = GetMousePosition();

        ui_context->begin(mouseDown, mousepos, dt);
        ui_context->push_theme(DEFAULT_THEME);

        auto root = ui::components::mk_root();

        Widget content({.mode = Children}, Size_Pct(1.f, 1.f),
                       GrowFlags::Column);

        Widget about_text(Size_Px(200.f, 0.5f), Size_Px(400.f, 0.5f));

        // NOTE: this is not aligned on purpose
        std::string about_info = R"(
A game by: 
    Gabe
    Brett
    Alice)";

        ui_context.get()->push_parent(root);
        {
            padding(*ui::components::mk_padding(Size_Px(100.f, 1.f),
                                                Size_FullH(0.f)));
            div(content);

            ui_context.get()->push_parent(&content);
            {
                padding(*ui::components::mk_padding(Size_Px(100.f, 1.f),
                                                    Size_Pct(0.5, 0.f)));
                text(about_text, about_info);
                if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID)),
                           "Back")) {
                    Menu::get().go_back();
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
        if (Menu::get().is_not(Menu::State::About)) return;
        SetExitKey(KEY_NULL);
    }

    virtual void onDraw(float dt) override {
        if (Menu::get().is_not(Menu::State::About)) return;
        ClearBackground(ui_context->active_theme().background);
        draw_ui(dt);
    }
};
