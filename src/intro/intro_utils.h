#pragma once

#include "engine/graphics.h"

namespace intro {

float animation_progress(float elapsed, float start_time, float duration);
raylib::Color white_alpha(float elapsed, float start, float duration);
raylib::Color fade_out(raylib::Color color, float elapsed,
                       float fade_start_time, float fade_duration);

}  // namespace intro
