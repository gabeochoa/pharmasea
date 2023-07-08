
#include "rendering_system.h"

#include "system_manager.h"

namespace system_manager {
namespace ui {

void render_networked_players(const Entities& entities, float dt) {
    float x_pos = WIN_WF() - 150;
    float y_pos = 20.f;

    auto _draw_text = [&](const std::string& str) mutable {
        int size = 20;
        DrawTextEx(Preload::get().font, str.c_str(), vec2{x_pos, y_pos}, size,
                   0, WHITE);
        y_pos += (size * 1.25f);
    };

    const auto _render_single_networked_player =
        [&](std::shared_ptr<Entity> entity, float) {
            _draw_text(                                     //
                fmt::format("{}({}) {}",                    //
                            entity->get<HasName>().name(),  //
                            entity->get<HasClientID>().id(),
                            // TODO replace with icon
                            entity->get<HasClientID>().ping()));
        };
    const auto _render_little_model_guy = [&](std::shared_ptr<Entity> entity,
                                              float) {
        auto model_name =
            entity->get<ModelRenderer>().model_info().value().model_name;
        raylib::Texture texture =
            TextureLibrary::get().get(fmt::format("{}_mug", model_name));
        float scale = 0.06f;
        raylib::DrawTextureEx(texture,
                              {x_pos - (500 /* image size */ * scale), y_pos},
                              0, scale, WHITE);
    };

    for (auto& entity : entities) {
        // TODO think about this check more
        if (!(check_name(entity, strings::entity::PLAYER) ||
              check_name(entity, strings::entity::REMOTE_PLAYER)))
            continue;
        _render_little_model_guy(entity, dt);
        _render_single_networked_player(entity, dt);
    }
}

}  // namespace ui
}  // namespace system_manager