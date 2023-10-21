
#pragma once

#include <vector>

#include "../graphics.h"

struct IUIContextRenderTextures {
    std::vector<raylib::RenderTexture2D> render_textures;

    [[nodiscard]] int get_new_render_texture() {
        raylib::RenderTexture2D tex =
            raylib::LoadRenderTexture(WIN_W(), WIN_H());
        render_textures.push_back(tex);
        return (int) render_textures.size() - 1;
    }

    void turn_on_render_texture(int rt_index) {
        BeginTextureMode(render_textures[rt_index]);
    }

    void turn_off_texture_mode() { raylib::EndTextureMode(); }

    void draw_texture(int rt_id, Rectangle source, vec2 position) {
        auto target = render_textures[rt_id];
        DrawTextureRec(target.texture, source, position, WHITE);
        // (Rectangle){0, 0, (float) target.texture.width,
        // (float) -target.texture.height},
        // (Vector2){0, 0}, WHITE);
    }

    void unload_all_render_textures() {
        // Note: we have to do this here because we cant unload until the entire
        // frame is written to the screen. We can guaranteed its definitely
        // rendered by the time it reaches the begin for the next frame
        for (auto target : render_textures) {
            raylib::UnloadRenderTexture(target);
        }
        render_textures.clear();
    }
};
