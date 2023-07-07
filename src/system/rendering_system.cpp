
#include "rendering_system.h"

#include "system_manager.h"

namespace system_manager {
namespace ui {

void render_networked_players(const Entities& entities, float dt) {
    float x_pos = WIN_WF() - 300;
    float y_pos = 200.f;

    auto _draw_text = [&](const std::string& str) mutable {
        DrawTextEx(Preload::get().font, str.c_str(), vec2{x_pos, y_pos}, 40, 0,
                   WHITE);
        y_pos += 50;
    };

    const auto _render_single_networked_player =
        [&](std::shared_ptr<Entity> entity, float) {
            _draw_text(                                     //
                fmt::format("{}({})",                       //
                            entity->get<HasName>().name(),  //
                            entity->get<HasClientID>().id()));
        };
    const auto _render_little_model_guy = [&](std::shared_ptr<Entity> entity,
                                              float) {
        auto model_name =
            entity->get<ModelRenderer>().model_info().value().model_name;
        raylib::Texture texture =
            TextureLibrary::get().get(fmt::format("{}_mug", model_name));
        float scale = 0.09f;
        raylib::DrawTextureEx(texture, {x_pos - (500 * scale), y_pos}, 0, scale,
                              WHITE);
    };

    for (auto& entity : entities) {
        // for (auto& entity : entities) {
        // TODO again pls make a better way
        if (entity->get<DebugName>().name() != "player" &&
            // TODO think about this harder
            entity->get<DebugName>().name() != "remote player")
            continue;
        _render_little_model_guy(entity, dt);
        _render_single_networked_player(entity, dt);
    }
}

}  // namespace ui
}  // namespace system_manager
