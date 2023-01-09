
#pragma once

#include <cmath>
#include <string>
#include <vector>

#include "engine/log.h"

struct ToastMsg {
    std::string_view msg;
    float timeToShow = 1.f;
    float timeHasShown = 0.f;

    float pctOpen = 1.f;

    void update(float dt) {
        timeHasShown = fmaxf(timeHasShown + (0.90f * dt), 0.f);
        pctOpen = 1.f - (timeHasShown / timeToShow);
    }
};

static std::vector<ToastMsg> TOASTS;

namespace toasts {

inline static void remove_complete() {
    auto it = TOASTS.begin();
    while (it != TOASTS.end()) {
        if ((*it).timeHasShown >= (*it).timeToShow) {
            TOASTS.erase(it);
            continue;
        }
        it++;
    }
}

inline static void update(float dt) {
    for (auto& toast_msg : TOASTS) {
        toast_msg.update(dt);
    }
    toasts::remove_complete();
}

}  // namespace toasts
