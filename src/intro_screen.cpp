#include "intro_screen.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>

#include "engine/files.h"
#include "engine/texture_library.h"
#include "globals.h"
#include "strings.h"

namespace {
constexpr float PRIMARY_MIN_TIME = 0.35F;
constexpr float PRIMARY_COMPLETE_HOLD = 0.25F;
constexpr float RAYLIB_ANIMATION_DURATION = 0.90F;
constexpr float RAYLIB_TOTAL_DURATION = RAYLIB_ANIMATION_DURATION * 4.5F;
constexpr float RAYLIB_FADE_DURATION = RAYLIB_ANIMATION_DURATION * 0.8F;
constexpr float RAYLIB_FADE_START = RAYLIB_ANIMATION_DURATION * 4.0F;
constexpr const char* POWERED_BY_TEXT = "POWERED BY";
constexpr const char* RAYLIB_TEXT = "raylib";
constexpr int CHOICEHONEY_FRAMES = 12;
constexpr float LOGO_TOTAL_DURATION = 1.5F;  // seconds for pepper animation
}  // namespace

IntroScreen::IntroScreen(const raylib::Font& font, bool show_raylib)
    : phase(show_raylib ? Phase::Raylib : Phase::Logo),
      show_raylib(show_raylib),
      primaryElapsed(0.0F),
      raylibElapsed(0.0F),
      holdAfterComplete(0.0F),
      started(false),
      finished(false),
      width(WIN_W()),
      height(WIN_H()),
      displayFont(font) {
    const std::string atlas_path =
        (Files::get().resource_folder() /
         std::filesystem::path("images/choicehoney_intro_atlas.png"))
            .string();
    if (std::filesystem::exists(atlas_path)) {
        choiceAtlas = raylib::LoadTexture(atlas_path.c_str());
        has_choicehoney = choiceAtlas.id > 0;
        if (!has_choicehoney) {
            log_warn("Failed to load choicehoney atlas {}", atlas_path);
        }
    } else {
        log_warn("Missing choicehoney atlas at {}", atlas_path);
    }
}

IntroScreen::~IntroScreen() {
    if (has_choicehoney) {
        raylib::UnloadTexture(choiceAtlas);
        has_choicehoney = false;
    }
}

void IntroScreen::start() {
    started = true;
    finished = false;
    phase = show_raylib ? Phase::Raylib : Phase::Logo;
    primaryElapsed = 0.0F;
    raylibElapsed = 0.0F;
    logoElapsed = 0.0F;
    holdAfterComplete = 0.0F;
    log_info("intro_screen start: show_raylib={}, phase={}", show_raylib,
             static_cast<int>(phase));
}

void IntroScreen::update(float progress) {
    if (finished) {
        return;
    }
    if (!started) {
        start();
    }

    float dt = raylib::GetFrameTime();

    if (phase == Phase::Raylib) {
        raylibElapsed += dt;
        if (!logged_raylib) {
            log_info("intro_screen raylib phase active, progress={}", progress);
            logged_raylib = true;
        }
        if (raylibElapsed > RAYLIB_TOTAL_DURATION) {
            phase = Phase::Logo;
            logoElapsed = 0.0F;
            log_info("intro_screen switched to logo phase");
        }
    } else if (phase == Phase::Logo) {
        logoElapsed += dt;
        if (!logged_logo) {
            log_info("intro_screen logo phase active");
            logged_logo = true;
        }
        if (logoElapsed >= LOGO_TOTAL_DURATION) {
            phase = Phase::Primary;
            primaryElapsed = 0.0F;
            holdAfterComplete = 0.0F;
            log_info("intro_screen switched to primary phase");
        }
    } else if (phase == Phase::Primary) {
        primaryElapsed += dt;
        if (progress >= 1.0F) {
            holdAfterComplete += dt;
        }
        if (progress >= 1.0F && primaryElapsed >= PRIMARY_MIN_TIME &&
            holdAfterComplete >= PRIMARY_COMPLETE_HOLD) {
            phase = Phase::Done;
            finished = true;
        }
    }

    draw(progress);
}

void IntroScreen::finish() {
    if (!started) {
        return;
    }
    if (finished) {
        return;
    }
    if (phase == Phase::Raylib) {
        phase = Phase::Logo;
        logoElapsed = LOGO_TOTAL_DURATION;
    }
    if (phase == Phase::Logo) {
        phase = Phase::Primary;
        primaryElapsed = PRIMARY_MIN_TIME;
        holdAfterComplete = PRIMARY_COMPLETE_HOLD;
    }
    if (phase == Phase::Primary) {
        draw(1.0F);
        finished = true;
        phase = Phase::Done;
    }
}

bool IntroScreen::is_raylib_active() const { return phase == Phase::Raylib; }
bool IntroScreen::is_logo_active() const { return phase == Phase::Logo; }

void IntroScreen::set_status_text(const std::string& text) {
    statusText = text;
}

void IntroScreen::draw(float progress) {
    raylib::BeginDrawing();
    raylib::ClearBackground(raylib::BLACK);

    if (phase == Phase::Primary) {
        draw_primary(progress);
    } else if (phase == Phase::Logo) {
        float logo_progress =
            std::clamp(logoElapsed / LOGO_TOTAL_DURATION, 0.0F, 1.0F);
        draw_logo(logo_progress);
    } else if (phase == Phase::Raylib) {
        draw_raylib();
    }

    raylib::EndDrawing();
}

void IntroScreen::draw_logo(float logo_progress) {
    float widthF = static_cast<float>(width);
    float heightF = static_cast<float>(height);
    if (!has_choicehoney || choiceAtlas.id == 0) {
        draw_primary(logo_progress);
        return;
    }

    int frameWidth = choiceAtlas.width / CHOICEHONEY_FRAMES;
    int frameHeight = choiceAtlas.height;
    int frameIndex =
        std::clamp(static_cast<int>(logo_progress * (CHOICEHONEY_FRAMES - 1)),
                   0, CHOICEHONEY_FRAMES - 1);
    raylib::Rectangle src{static_cast<float>(frameIndex * frameWidth), 0,
                          static_cast<float>(frameWidth),
                          static_cast<float>(frameHeight)};
    float scale = std::min(widthF * 0.65F / static_cast<float>(frameWidth),
                           heightF * 0.4F / static_cast<float>(frameHeight));
    float destW = static_cast<float>(frameWidth) * scale;
    float destH = static_cast<float>(frameHeight) * scale;
    vec2 destPos{widthF * 0.5F - destW * 0.5F, heightF * 0.5F - destH * 0.5F};
    raylib::Rectangle dest{destPos.x, destPos.y, destW, destH};
    raylib::DrawTexturePro(choiceAtlas, src, dest, vec2{0, 0}, 0.0F,
                           raylib::WHITE);
}

void IntroScreen::draw_primary(float progress) {
    float widthF = static_cast<float>(width);
    float heightF = static_cast<float>(height);

    std::string title = strings::GAME_NAME;
    float titleSize = heightF / 10.0F;
    vec2 titleSizeVec =
        raylib::MeasureTextEx(displayFont, title.c_str(), titleSize, 1.0F);
    vec2 titlePos{widthF * 0.5F - titleSizeVec.x * 0.5F,
                  heightF * 0.38F - titleSizeVec.y * 0.5F};
    float pulse = std::sin(primaryElapsed * 2.4F) * 0.5F + 0.5F;
    pulse = std::clamp(pulse, 0.0F, 1.0F);
    unsigned char baseAlpha =
        static_cast<unsigned char>(200.0F + 55.0F * pulse);
    raylib::Color primaryColor{255, 255, 255, baseAlpha};
    raylib::DrawTextEx(displayFont, title.c_str(), titlePos, titleSize, 1.0F,
                       primaryColor);
    const char* loadingText = statusText.c_str();
    float infoSize = titleSize * 0.25F;
    vec2 infoSizeVec =
        raylib::MeasureTextEx(displayFont, loadingText, infoSize, 1.0F);
    vec2 infoPos{widthF * 0.5F - infoSizeVec.x * 0.5F,
                 titlePos.y + titleSizeVec.y + infoSize * 0.8F};
    raylib::DrawTextEx(displayFont, loadingText, infoPos, infoSize, 1.0F,
                       primaryColor);
    float clampedProgress = std::clamp(progress, 0.0F, 1.0F);
    float barWidth = widthF * 0.5F;
    float barHeight = heightF * 0.02F;
    vec2 barPos{widthF * 0.25F, heightF * 0.65F};
    raylib::DrawRectangleLinesEx(
        raylib::Rectangle{barPos.x, barPos.y, barWidth, barHeight}, 3.0F,
        primaryColor);
    raylib::Color fillColor{100, 220, 255, 255};
    float fillWidth = barWidth * clampedProgress;
    raylib::DrawRectangle(static_cast<int>(barPos.x),
                          static_cast<int>(barPos.y),
                          static_cast<int>(fillWidth),
                          static_cast<int>(barHeight), fillColor);
}

void IntroScreen::draw_raylib() {
    int fontSizeInt = static_cast<int>(static_cast<float>(height) / 15.0F);
    float fontSize = static_cast<float>(fontSizeInt);

    vec2 startPosition{static_cast<float>(width) * 0.4F, fontSize * 4.0F};
    vec2 boxTopLeft{startPosition.x, startPosition.y + fontSize * 1.5F};

    render_powered_by_text(displayFont, startPosition, fontSize,
                           RAYLIB_FADE_START, RAYLIB_FADE_DURATION);

    float poweredWidth = raylib::MeasureText(POWERED_BY_TEXT, fontSizeInt);
    float boxWidth = poweredWidth * 0.80F;
    render_box_lines(boxTopLeft, boxWidth, RAYLIB_FADE_START,
                     RAYLIB_FADE_DURATION);
    render_raylib_text(displayFont, boxTopLeft, fontSize, RAYLIB_FADE_START,
                       RAYLIB_FADE_DURATION);
}

raylib::Color IntroScreen::white_alpha(float start, float duration) const {
    float alphaT = std::clamp((raylibElapsed - start) / duration, 0.0F, 1.0F);
    float alphaF = std::lerp(0.0F, 255.0F, alphaT);
    unsigned char alpha =
        static_cast<unsigned char>(std::clamp(alphaF, 0.0F, 255.0F));
    return {255, 255, 255, alpha};
}

float IntroScreen::animation_progress(float start_time, float duration) const {
    float elapsedTime = raylibElapsed - start_time;
    if (elapsedTime <= 0.0F) {
        return 0.0F;
    }
    if (elapsedTime >= duration) {
        return 1.0F;
    }
    return elapsedTime / duration;
}

void IntroScreen::render_powered_by_text(raylib::Font& font,
                                         const vec2& position, float font_size,
                                         float fade_start_time,
                                         float fade_duration) {
    raylib::Color color = white_alpha(0.0F, RAYLIB_ANIMATION_DURATION);
    if (raylibElapsed > fade_start_time) {
        float fadeProgress = animation_progress(fade_start_time, fade_duration);
        float scaled = static_cast<float>(color.a) * (1.0F - fadeProgress);
        color.a = static_cast<unsigned char>(scaled);
    }
    vec2 pos{position.x - font_size / 4.0F, position.y};
    raylib::DrawTextEx(font, POWERED_BY_TEXT, pos, font_size, 1.0F, color);
}

void IntroScreen::render_box_lines(const vec2& box_top_left,
                                   float box_line_width, float fade_start_time,
                                   float fade_duration) {
    if (raylibElapsed > RAYLIB_ANIMATION_DURATION) {
        float pctComplete = animation_progress(RAYLIB_ANIMATION_DURATION,
                                               RAYLIB_ANIMATION_DURATION);
        raylib::Color lineColor =
            white_alpha(RAYLIB_ANIMATION_DURATION, RAYLIB_ANIMATION_DURATION);
        if (raylibElapsed > fade_start_time) {
            float fadeProgress =
                animation_progress(fade_start_time, fade_duration);
            float scaled =
                static_cast<float>(lineColor.a) * (1.0F - fadeProgress);
            lineColor.a = static_cast<unsigned char>(scaled);
        }
        vec2 endTop{box_top_left.x + box_line_width * pctComplete,
                    box_top_left.y};
        vec2 endLeft{box_top_left.x,
                     box_top_left.y + box_line_width * pctComplete};
        raylib::DrawLineEx(box_top_left, endTop, 5.0F, lineColor);
        raylib::DrawLineEx(box_top_left, endLeft, 5.0F, lineColor);
    }

    if (raylibElapsed > (RAYLIB_ANIMATION_DURATION * 2.0F)) {
        float pctComplete = animation_progress(RAYLIB_ANIMATION_DURATION * 2.0F,
                                               RAYLIB_ANIMATION_DURATION);
        raylib::Color lineColor =
            white_alpha(RAYLIB_ANIMATION_DURATION, RAYLIB_ANIMATION_DURATION);
        if (raylibElapsed > fade_start_time) {
            float fadeProgress =
                animation_progress(fade_start_time, fade_duration);
            float scaled =
                static_cast<float>(lineColor.a) * (1.0F - fadeProgress);
            lineColor.a = static_cast<unsigned char>(scaled);
        }

        vec2 right{box_top_left.x + box_line_width, box_top_left.y};
        vec2 rightEnd{right.x, right.y + box_line_width * pctComplete};
        raylib::DrawLineEx(right, rightEnd, 5.0F, lineColor);

        vec2 bottom{box_top_left.x, box_top_left.y + box_line_width};
        vec2 bottomEnd{bottom.x + box_line_width * pctComplete, bottom.y};
        raylib::DrawLineEx(bottom, bottomEnd, 5.0F, lineColor);
    }
}

void IntroScreen::render_raylib_text(raylib::Font& font,
                                     const vec2& box_top_left, float font_size,
                                     float fade_start_time,
                                     float fade_duration) {
    if (raylibElapsed > (RAYLIB_ANIMATION_DURATION * 3.0F)) {
        int fontSizeInt = static_cast<int>(font_size);
        float poweredWidth = static_cast<float>(
            raylib::MeasureText(POWERED_BY_TEXT, fontSizeInt));
        float box_line_width = poweredWidth * 0.80F;
        vec2 boxBottomRight{box_top_left.x + box_line_width,
                            box_top_left.y + box_line_width};
        float raylibWidth =
            static_cast<float>(raylib::MeasureText(RAYLIB_TEXT, fontSizeInt));

        raylib::Color textColor = white_alpha(RAYLIB_ANIMATION_DURATION * 3.0F,
                                              RAYLIB_ANIMATION_DURATION * 3.0F);
        if (raylibElapsed > fade_start_time) {
            float fadeProgress =
                animation_progress(fade_start_time, fade_duration);
            float scaled =
                static_cast<float>(textColor.a) * (1.0F - fadeProgress);
            textColor.a = static_cast<unsigned char>(scaled);
        }

        vec2 pos{boxBottomRight.x - raylibWidth, boxBottomRight.y - font_size};
        raylib::DrawTextEx(font, RAYLIB_TEXT, pos, font_size, 1.0F, textColor);
    }
}
