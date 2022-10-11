
#pragma once

#include "app.h"
#include "event.h"
#include "external_include.h"
#include "globals.h"
#include "singleton.h"

SINGLETON_FWD(Settings)

struct Settings {
    SINGLETON(Settings)

    vec2 window_size = {WIN_W, WIN_H};
    // Volume percent [0, 1] for everything
    float masterVolume = 0.5f;

    void update_window_size(vec2 size) {
        window_size = size;
        WindowResizeEvent* event = new WindowResizeEvent(
            static_cast<int>(size.x), static_cast<int>(size.y));
        App::get().processEvent(*event);
        delete event;
    }

    void update_master_volume(float nv) {
        masterVolume = nv;
        // TODO sent out audio volume change event
    }
};
