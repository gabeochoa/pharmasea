
#pragma once

#include "../external_include.h"
#include "ui/ui.h"

static std::atomic_int s_layer_id;
struct Layer {
    // TODO it would be great to do this here instead of in every sub layer but
    // theres some problem with including ui.h in here due to app.cpp
    // std::shared_ptr<ui::UIContext> ui_context;
    int id;
    std::string name;

    explicit Layer(const std::string& n = "layer")
        : id(s_layer_id++), name(n) {}
    virtual ~Layer() {}
    virtual void onStartup() {}
    virtual void onUpdate(float elapsed) = 0;
    virtual void onDraw(float elapsed) = 0;

    [[nodiscard]] const std::string& getname() const { return name; }
};
