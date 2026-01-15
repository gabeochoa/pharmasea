#pragma once

#include <string>

struct IntroScene {
    virtual ~IntroScene() = default;

    virtual bool update(float dt, float external_progress) = 0;
    virtual void set_status_text(const std::string& text) { (void) text; }
    virtual void finish() {}
};
