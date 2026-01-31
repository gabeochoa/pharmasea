#pragma once

#include <memory>
#include <vector>

#include "intro_scene.h"

class IntroRunner {
   public:
    explicit IntroRunner(std::vector<std::unique_ptr<IntroScene>> scenes);

    bool update(float dt, float external_progress);
    void set_status_text(const std::string& text);
    void finish_all();
    bool empty() const;

   private:
    std::vector<std::unique_ptr<IntroScene>> scenes;
};
