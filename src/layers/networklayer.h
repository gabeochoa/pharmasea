
#pragma once

#include <memory>

#include "../engine.h"
#include "../external_include.h"
//
#include "../globals.h"
//
#include "../engine/layer.h"
#include "../engine/svg_renderer.h"
#include "../local_ui.h"

using namespace ui;

bool validate_ip(const std::string& ip);

struct NetworkLayer : public Layer {
    std::shared_ptr<ui::UIContext> ui_context;
    SVGRenderer lobby_screen;
    SVGRenderer join_lobby_screen;
    SVGRenderer username_screen;
    SVGRenderer network_selection_screen;

    std::string my_ip_address;
    bool should_show_host_ip = false;

    NetworkLayer();

    virtual void onStartup() override;
    virtual ~NetworkLayer();

    void handleInput();
    virtual void onUpdate(float dt) override;
    void handle_announcements();

    void draw_username_picker(float);
    void draw_username_with_edit(Rectangle username, float);
    void draw_role_selector_screen(float);
    void draw_connected_screen(float);
    void draw_ip_input_screen(float);
    void draw_screen(float dt);
    virtual void onDraw(float dt) override;
};
