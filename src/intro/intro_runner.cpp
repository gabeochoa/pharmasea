#include "intro_runner.h"

#include <utility>

IntroRunner::IntroRunner(std::vector<std::unique_ptr<IntroScene>> scenes_in)
    : scenes(std::move(scenes_in)) {}

bool IntroRunner::update(float dt, float external_progress) {
    if (scenes.empty()) {
        return true;
    }

    while (!scenes.empty()) {
        std::unique_ptr<IntroScene>& current = scenes.front();
        bool completed = current->update(dt, external_progress);
        if (completed) {
            scenes.erase(scenes.begin());
            continue;
        }
        break;
    }

    return scenes.empty();
}

void IntroRunner::set_status_text(const std::string& text) {
    if (scenes.empty()) {
        return;
    }
    scenes.front()->set_status_text(text);
}

void IntroRunner::finish_all() {
    for (std::unique_ptr<IntroScene>& scene : scenes) {
        scene->finish();
    }
    scenes.clear();
}

bool IntroRunner::empty() const { return scenes.empty(); }
