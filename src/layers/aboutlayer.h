
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
          ui_context(std::make_shared<ui::UIContext>()) {
        root_box =
            load_ui("resources/html/about.html", {0, 0, WIN_WF(), WIN_HF()});
    }
    virtual ~AboutLayer() {}

    bool onKeyPressed(KeyPressedEvent& event) override {
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
                text(*about_text, text_lookup(strings::ABOUT_INFO));
                if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID)),
                           text_lookup(strings::i18n::BACK_BUTTON))) {
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

    void process_on_click(const std::string& id) {
        switch (hashString(id)) {
            case hashString(strings::i18n::BACK_BUTTON):
                MenuState::get().set(menu::State::Root);
                break;
        }
    }

    virtual void onDraw(float) override {
        if (MenuState::get().is_not(menu::State::About)) return;
        ext::clear_background(ui_context->active_theme().background);

        elements::focus::begin();

        render_ui(ui_context, root_box, {0, 0, WIN_WF(), WIN_HF()},
                  std::bind(&AboutLayer::process_on_click, *this,
                            std::placeholders::_1));

        elements::focus::end();
    }
};
