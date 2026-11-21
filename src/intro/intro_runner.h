#pragma once

#include <memory>
#include <vector>

#include "intro_scene.h"
#include "logo_intro_scene.h"
#include "raylib_intro_scene.h"

class IntroRunner {
   public:
    enum class IntroPhase { Logo, Raylib, Loading, Done };

    explicit IntroRunner(std::vector<std::unique_ptr<IntroScene>> scenes);

    bool update(float dt, float external_progress);
    void set_status_text(const std::string& text);
    void finish_all();
    bool empty() const;

    [[nodiscard]] IntroPhase phase() const {
        if (scenes.empty()) return IntroPhase::Done;
        if (dynamic_cast<LogoIntroScene*>(scenes.front().get())) {
            return IntroPhase::Logo;
        }
        if (dynamic_cast<RaylibIntroScene*>(scenes.front().get())) {
            return IntroPhase::Raylib;
        }
        return IntroPhase::Loading;
    }

    [[nodiscard]] bool is_phase(IntroPhase p) const { return phase() == p; }

   private:
    std::vector<std::unique_ptr<IntroScene>> scenes;
    IntroPhase last_phase = IntroPhase::Done;
};
