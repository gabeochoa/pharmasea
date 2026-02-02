#pragma once

#include "engine/graphics.h"
#include "intro_scene.h"

class RaylibIntroScene : public IntroScene {
   public:
    explicit RaylibIntroScene(const raylib::Font& font);

    bool update(float dt, float external_progress) override;
    void finish() override;

   private:
    raylib::Font displayFont{};
    int width = 0;
    int height = 0;
    float elapsed = 0.0F;

    void draw() const;
    void render_powered_by_text(const vec2& position, float font_size) const;
    void render_box_lines(const vec2& box_top_left, float box_line_width) const;
    void render_raylib_text(const vec2& box_top_left, float font_size) const;
};
