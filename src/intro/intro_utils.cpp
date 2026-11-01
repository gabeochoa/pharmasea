#include "intro_utils.h"

#include <algorithm>
#include <cmath>

namespace intro {

float animation_progress(float elapsed, float start_time, float duration) {
    float elapsed_time = elapsed - start_time;
    if (elapsed_time <= 0.0F) {
        return 0.0F;
    }
    if (elapsed_time >= duration) {
        return 1.0F;
    }
    return elapsed_time / duration;
}

raylib::Color white_alpha(float elapsed, float start, float duration) {
    float alpha_t = std::clamp((elapsed - start) / duration, 0.0F, 1.0F);
    float alpha_f = std::lerp(0.0F, 255.0F, alpha_t);
    unsigned char alpha =
        static_cast<unsigned char>(std::clamp(alpha_f, 0.0F, 255.0F));
    return {255, 255, 255, alpha};
}

raylib::Color fade_out(raylib::Color color, float elapsed,
                       float fade_start_time, float fade_duration) {
    if (elapsed <= fade_start_time) {
        return color;
    }
    float fade_progress =
        animation_progress(elapsed, fade_start_time, fade_duration);
    float scaled_alpha = static_cast<float>(color.a) * (1.0F - fade_progress);
    color.a = static_cast<unsigned char>(scaled_alpha);
    return color;
}

}  // namespace intro
