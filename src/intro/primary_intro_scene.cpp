#include "primary_intro_scene.h"

#include <algorithm>
#include <cmath>

#include "globals.h"
#include "strings.h"

namespace {
constexpr float PRIMARY_MIN_TIME = 0.35F;
constexpr float PRIMARY_COMPLETE_HOLD = 0.25F;
}  // namespace

PrimaryIntroScene::PrimaryIntroScene(const raylib::Font& font)
    : displayFont(font), width(WIN_W()), height(WIN_H()), elapsed(0.0F) {}

bool PrimaryIntroScene::update(float dt, float external_progress) {
    elapsed += dt;
    float clamped_progress = std::clamp(external_progress, 0.0F, 1.0F);
    if (clamped_progress >= 1.0F) {
        holdAfterComplete += dt;
    }

    raylib::BeginDrawing();
    raylib::ClearBackground(raylib::BLACK);
    draw(clamped_progress);
    raylib::EndDrawing();

    if (clamped_progress >= 1.0F && elapsed >= PRIMARY_MIN_TIME &&
        holdAfterComplete >= PRIMARY_COMPLETE_HOLD) {
        return true;
    }
    return false;
}

void PrimaryIntroScene::set_status_text(const std::string& text) {
    statusText = text;
}

void PrimaryIntroScene::finish() {
    elapsed = PRIMARY_MIN_TIME;
    holdAfterComplete = PRIMARY_COMPLETE_HOLD;
}

void PrimaryIntroScene::draw(float progress) {
    float width_f = static_cast<float>(width);
    float height_f = static_cast<float>(height);

    std::string title = strings::GAME_NAME;
    float titleSize = height_f / 10.0F;
    vec2 titleSizeVec =
        raylib::MeasureTextEx(displayFont, title.c_str(), titleSize, 1.0F);
    vec2 titlePos{width_f * 0.5F - titleSizeVec.x * 0.5F,
                  height_f * 0.38F - titleSizeVec.y * 0.5F};
    float pulse = std::sin(elapsed * 2.4F) * 0.5F + 0.5F;
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
    vec2 infoPos{width_f * 0.5F - infoSizeVec.x * 0.5F,
                 titlePos.y + titleSizeVec.y + infoSize * 0.8F};
    raylib::DrawTextEx(displayFont, loadingText, infoPos, infoSize, 1.0F,
                       primaryColor);
    float barWidth = width_f * 0.5F;
    float barHeight = height_f * 0.02F;
    vec2 barPos{width_f * 0.25F, height_f * 0.65F};
    raylib::DrawRectangleLinesEx(
        raylib::Rectangle{barPos.x, barPos.y, barWidth, barHeight}, 3.0F,
        primaryColor);
    raylib::Color fillColor{100, 220, 255, 255};
    float fillWidth = barWidth * progress;
    raylib::DrawRectangle(
        static_cast<int>(barPos.x), static_cast<int>(barPos.y),
        static_cast<int>(fillWidth), static_cast<int>(barHeight), fillColor);
}
