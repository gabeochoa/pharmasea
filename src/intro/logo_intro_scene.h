#pragma once

#include "engine/graphics.h"
#include "intro_scene.h"

class LogoIntroScene : public IntroScene {
   public:
    LogoIntroScene();
    ~LogoIntroScene();

    bool update(float dt, float external_progress) override;
    void finish() override;

   private:
    raylib::Texture atlas{};
    bool has_atlas = false;
    int width = 0;
    int height = 0;
    float elapsed = 0.0F;

    void load_atlas();
    bool draw_logo() const;
};
