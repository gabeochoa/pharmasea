#pragma once

#include "engine/graphics.h"

struct IntroScreen {
    enum struct Phase { Primary, Raylib, Done };

    explicit IntroScreen(const raylib::Font& font, bool show_raylib);

    void start();
    void update(float progress);
    void finish();
    bool is_raylib_active() const;
    void set_status_text(const std::string& text);

   private:
    Phase phase = Phase::Primary;
    bool show_raylib = true;
    bool logged_raylib = false;
    float primaryElapsed = 0.0F;
    float raylibElapsed = 0.0F;
    float holdAfterComplete = 0.0F;
    bool started = false;
    bool finished = false;
    int width = 0;
    int height = 0;
    raylib::Font displayFont{};
    std::string statusText = "Loading models";

    void draw(float progress);
    void draw_primary(float progress);
    void draw_raylib();
    raylib::Color white_alpha(float start, float duration) const;
    float animation_progress(float start_time, float duration) const;
    void render_powered_by_text(raylib::Font& font, const vec2& position,
                                float font_size, float fade_start_time,
                                float fade_duration);
    void render_box_lines(const vec2& box_top_left, float width,
                          float fade_start_time, float fade_duration);
    void render_raylib_text(raylib::Font& font, const vec2& box_top_left,
                            float font_size, float fade_start_time,
                            float fade_duration);
};
