
#pragma once

#include "../engine.h"
#include "../entity.h"
#include "../external_include.h"
#include "../preload.h"

struct PlayerInfoLayer : public Layer {
    PlayerInfoLayer() : Layer("PlayerInfo") {}

    int y_pos = 0;

    virtual ~PlayerInfoLayer() {}
    virtual void onEvent(Event&) override {}
    virtual void onUpdate(float) override {}

    void draw_text(const std::string& str) {
        float y = 100 + y_pos;
        DrawTextEx(Preload::get().font, str.c_str(), vec2{5, y}, 20, 0, WHITE);
        y_pos += 15;
    }

    virtual void onDraw(float) override {
        Entity* me = GLOBALS.get_ptr<Entity>("player");
        if (!me) return;

        y_pos = 0;
        draw_text("PlayerInfo:");
        draw_text(fmt::format("position: {}", me->get<Transform>().pos()));
        draw_text(fmt::format("holding item?: {}",
                              me->get<CanHoldItem>().is_holding_item()));
    }
};
