#include "logo_intro_scene.h"

#include <algorithm>
#include <filesystem>

#include "engine/files.h"
#include "globals.h"

namespace {
constexpr int CHOICEHONEY_FRAMES = 52;
constexpr int CHOICEHONEY_COLUMNS = 5;
constexpr float LOGO_TOTAL_DURATION = 1.5F;
}  // namespace

LogoIntroScene::LogoIntroScene()
    : atlas(),
      has_atlas(false),
      width(WIN_W()),
      height(WIN_H()),
      elapsed(0.0F) {
    load_atlas();
}

LogoIntroScene::~LogoIntroScene() {
    if (has_atlas) {
        raylib::UnloadTexture(atlas);
        has_atlas = false;
    }
}

bool LogoIntroScene::update(float dt, float) {
    elapsed += dt;

    raylib::BeginDrawing();
    raylib::ClearBackground(raylib::BLACK);
    bool drew_logo = draw_logo();
    raylib::EndDrawing();

    if (!drew_logo) {
        return true;
    }

    return elapsed >= LOGO_TOTAL_DURATION;
}

void LogoIntroScene::finish() { elapsed = LOGO_TOTAL_DURATION; }

void LogoIntroScene::load_atlas() {
    const std::string atlas_path =
        (Files::get().resource_folder() /
         std::filesystem::path("images/playful_intro_atlas.png"))
            .string();
    if (std::filesystem::exists(atlas_path)) {
        atlas = raylib::LoadTexture(atlas_path.c_str());
        has_atlas = atlas.id > 0;
        if (!has_atlas) {
            log_warn("Failed to load choicehoney atlas {}", atlas_path);
        } else {
            log_info("Loaded intro atlas {} size {}x{} frames {}", atlas_path,
                     atlas.width, atlas.height, CHOICEHONEY_FRAMES);
        }
    } else {
        log_warn("Missing choicehoney atlas at {}", atlas_path);
    }
}

bool LogoIntroScene::draw_logo() const {
    if (!has_atlas || atlas.id == 0) {
        return false;
    }

    float width_f = static_cast<float>(width);
    float height_f = static_cast<float>(height);

    float logo_progress = std::clamp(elapsed / LOGO_TOTAL_DURATION, 0.0F, 1.0F);

    int frame_index =
        std::clamp(static_cast<int>(logo_progress * (CHOICEHONEY_FRAMES - 1)),
                   0, CHOICEHONEY_FRAMES - 1);
    int columns = std::max(1, CHOICEHONEY_COLUMNS);
    int rows = (CHOICEHONEY_FRAMES + columns - 1) / columns;
    int frame_width = atlas.width / columns;
    int frame_height = atlas.height / rows;
    int col = frame_index % columns;
    int row = frame_index / columns;
    raylib::Rectangle src{static_cast<float>(col * frame_width),
                          static_cast<float>(row * frame_height),
                          static_cast<float>(frame_width),
                          static_cast<float>(frame_height)};
    float scale = std::min(width_f * 0.65F / static_cast<float>(frame_width),
                           height_f * 0.4F / static_cast<float>(frame_height));
    float dest_w = static_cast<float>(frame_width) * scale;
    float dest_h = static_cast<float>(frame_height) * scale;
    vec2 dest_pos{width_f * 0.5F - dest_w * 0.5F,
                  height_f * 0.5F - dest_h * 0.5F};
    raylib::Rectangle dest{dest_pos.x, dest_pos.y, dest_w, dest_h};
    raylib::DrawTexturePro(atlas, src, dest, vec2{0, 0}, 0.0F, raylib::WHITE);
    return true;
}
