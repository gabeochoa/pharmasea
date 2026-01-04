#pragma once

#include <string>

#include "engine/graphics.h"
#include "intro_scene.h"

class PrimaryIntroScene : public IntroScene {
   public:
    explicit PrimaryIntroScene(const raylib::Font& font);

    bool update(float dt, float external_progress) override;
    void set_status_text(const std::string& text) override;
    void finish() override;

   private:
    raylib::Font displayFont{};
    int width = 0;
    int height = 0;
    float elapsed = 0.0F;
    float holdAfterComplete = 0.0F;
    std::string statusText = "Loading models";

    void draw(float progress);
};
