
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

    void update_window_size(vec2 size) {
        window_size = size;
        WindowResizeEvent* event = new WindowResizeEvent(size.x, size.y);
        App::get().processEvent(*event);
        delete event;
    }
};
