#include "raylib_intro_scene.h"

#include <algorithm>

#include "globals.h"
#include "intro_utils.h"

namespace {
constexpr float RAYLIB_ANIMATION_DURATION = 0.90F;
constexpr float RAYLIB_TOTAL_DURATION = RAYLIB_ANIMATION_DURATION * 4.5F;
constexpr float RAYLIB_FADE_DURATION = RAYLIB_ANIMATION_DURATION * 0.8F;
constexpr float RAYLIB_FADE_START = RAYLIB_ANIMATION_DURATION * 4.0F;
constexpr const char* POWERED_BY_TEXT = "POWERED BY";
constexpr const char* RAYLIB_TEXT = "raylib";
}  // namespace

RaylibIntroScene::RaylibIntroScene(const raylib::Font& font)
    : displayFont(font), width(WIN_W()), height(WIN_H()), elapsed(0.0F) {}

bool RaylibIntroScene::update(float dt, float) {
    elapsed += dt;

    raylib::BeginDrawing();
    raylib::ClearBackground(raylib::BLACK);
    draw();
    raylib::EndDrawing();

    return elapsed >= RAYLIB_TOTAL_DURATION;
}

void RaylibIntroScene::finish() { elapsed = RAYLIB_TOTAL_DURATION; }

void RaylibIntroScene::draw() const {
    int fontSizeInt = static_cast<int>(static_cast<float>(height) / 15.0F);
    float fontSize = static_cast<float>(fontSizeInt);

    vec2 startPosition{static_cast<float>(width) * 0.4F, fontSize * 4.0F};
    vec2 boxTopLeft{startPosition.x, startPosition.y + fontSize * 1.5F};

    render_powered_by_text(startPosition, fontSize);

    float poweredWidth =
        static_cast<float>(raylib::MeasureText(POWERED_BY_TEXT, fontSizeInt));
    float boxWidth = poweredWidth * 0.80F;
    render_box_lines(boxTopLeft, boxWidth);
    render_raylib_text(boxTopLeft, fontSize);
}

void RaylibIntroScene::render_powered_by_text(const vec2& position,
                                              float font_size) const {
    raylib::Color color =
        intro::white_alpha(elapsed, 0.0F, RAYLIB_ANIMATION_DURATION);
    color = intro::fade_out(color, elapsed, RAYLIB_FADE_START,
                            RAYLIB_FADE_DURATION);
    vec2 pos{position.x - font_size / 4.0F, position.y};
    raylib::DrawTextEx(displayFont, POWERED_BY_TEXT, pos, font_size, 1.0F,
                       color);
}

void RaylibIntroScene::render_box_lines(const vec2& box_top_left,
                                        float box_line_width) const {
    if (elapsed > RAYLIB_ANIMATION_DURATION) {
        float pct_complete = intro::animation_progress(
            elapsed, RAYLIB_ANIMATION_DURATION, RAYLIB_ANIMATION_DURATION);
        raylib::Color lineColor = intro::white_alpha(
            elapsed, RAYLIB_ANIMATION_DURATION, RAYLIB_ANIMATION_DURATION);
        lineColor = intro::fade_out(lineColor, elapsed, RAYLIB_FADE_START,
                                    RAYLIB_FADE_DURATION);
        vec2 endTop{box_top_left.x + box_line_width * pct_complete,
                    box_top_left.y};
        vec2 endLeft{box_top_left.x,
                     box_top_left.y + box_line_width * pct_complete};
        raylib::DrawLineEx(box_top_left, endTop, 5.0F, lineColor);
        raylib::DrawLineEx(box_top_left, endLeft, 5.0F, lineColor);
    }

    if (elapsed > (RAYLIB_ANIMATION_DURATION * 2.0F)) {
        float pct_complete =
            intro::animation_progress(elapsed, RAYLIB_ANIMATION_DURATION * 2.0F,
                                      RAYLIB_ANIMATION_DURATION);
        raylib::Color lineColor = intro::white_alpha(
            elapsed, RAYLIB_ANIMATION_DURATION, RAYLIB_ANIMATION_DURATION);
        lineColor = intro::fade_out(lineColor, elapsed, RAYLIB_FADE_START,
                                    RAYLIB_FADE_DURATION);

        vec2 right{box_top_left.x + box_line_width, box_top_left.y};
        vec2 rightEnd{right.x, right.y + box_line_width * pct_complete};
        raylib::DrawLineEx(right, rightEnd, 5.0F, lineColor);

        vec2 bottom{box_top_left.x, box_top_left.y + box_line_width};
        vec2 bottomEnd{bottom.x + box_line_width * pct_complete, bottom.y};
        raylib::DrawLineEx(bottom, bottomEnd, 5.0F, lineColor);
    }
}

void RaylibIntroScene::render_raylib_text(const vec2& box_top_left,
                                          float font_size) const {
    if (elapsed > (RAYLIB_ANIMATION_DURATION * 3.0F)) {
        int fontSizeInt = static_cast<int>(font_size);
        float poweredWidth = static_cast<float>(
            raylib::MeasureText(POWERED_BY_TEXT, fontSizeInt));
        float box_line_width = poweredWidth * 0.80F;
        vec2 boxBottomRight{box_top_left.x + box_line_width,
                            box_top_left.y + box_line_width};
        float raylibWidth =
            static_cast<float>(raylib::MeasureText(RAYLIB_TEXT, fontSizeInt));

        raylib::Color textColor =
            intro::white_alpha(elapsed, RAYLIB_ANIMATION_DURATION * 3.0F,
                               RAYLIB_ANIMATION_DURATION * 3.0F);
        textColor = intro::fade_out(textColor, elapsed, RAYLIB_FADE_START,
                                    RAYLIB_FADE_DURATION);

        vec2 pos{boxBottomRight.x - raylibWidth, boxBottomRight.y - font_size};
        raylib::DrawTextEx(displayFont, RAYLIB_TEXT, pos, font_size, 1.0F,
                           textColor);
    }
}
