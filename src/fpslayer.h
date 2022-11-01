#pragma once

#include "external_include.h"
#include "globals.h"
#include "layer.h"
#include "preload.h"
#include "profile.h"
#include "raylib.h"

using namespace profile;
struct FPSLayer : public Layer {
    FPSLayer() : Layer("FPS") {
        minimized = false;
        Profiler::clear();
    }
    virtual ~FPSLayer() {}
    virtual void onAttach() override {}
    virtual void onDetach() override {}
    virtual void onEvent(Event&) override {}
    virtual void onUpdate(float) override {}

    virtual void onDraw(float) override {
        // TODO with gamelayer, support events
        if (minimized) {
            return;
        }
        DrawFPS(0, 0);

        if (GLOBALS.get<bool>("debug_ui_enabled")) {
            std::vector<Sample> pairs;
            pairs.insert(pairs.end(), Profiler::get()._acc.begin(),
                         Profiler::get()._acc.end());

            sort(pairs.begin(), pairs.end(),
                 [](const Sample& a, const Sample& b) {
                     return a.second.average() > b.second.average();
                 });

            int ypos = 150;
            int spacing = 20;
            for (const auto& kv : pairs) {
                auto stats = kv.second;
                std::string stat_str =
                    fmt::format("{}{}: avg {:.0f}ns", stats.filename, kv.first,
                                stats.average());
                int string_width = MeasureText(stat_str.c_str(), 15);
                DrawRectangle(100, ypos, string_width, 20, BLACK);
                DrawTextEx(Preload::get().font, stat_str.c_str(),
                           vec2{100, (float) ypos}, 20, 0, WHITE);
                ypos += spacing;
            }
        }
    }
};
