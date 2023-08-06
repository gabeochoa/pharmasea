
#include "rendering_system.h"

#include "../components/has_client_id.h"
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

    const auto _render_single_networked_player = [&](const Entity& entity,
                                                     float) {
        _draw_text(                                    //
            fmt::format("{}({}) {}",                   //
                        entity.get<HasName>().name(),  //
                        entity.get<HasClientID>().id(),
                        // TODO replace with icon
                        entity.get<HasClientID>().ping()));
    };
    const auto _render_little_model_guy = [&](const Entity& entity, float) {
        if (entity.is_missing<ModelRenderer>()) {
            log_warn(
                "render_little_model_guy, entity {} is missing model renderer",
                entity.get<DebugName>().name());
            return;
        }
        auto model_name = entity.get<ModelRenderer>().name();
        raylib::Texture texture =
            TextureLibrary::get().get(fmt::format("{}_mug", model_name));
        float scale = 0.06f;
        raylib::DrawTextureEx(texture,
                              {x_pos - (500 /* image size */ * scale), y_pos},
                              0, scale, WHITE);
    };

    for (const auto& entity_ptr : entities) {
        if (!entity_ptr) continue;
        Entity& entity = *entity_ptr;
        // TODO think about this check more
        if (!(check_name(entity, strings::entity::PLAYER) ||
              check_name(entity, strings::entity::REMOTE_PLAYER)))
            continue;
        _render_little_model_guy(entity, dt);
        _render_single_networked_player(entity, dt);
    }
}

void render_player_info(const Entity& entity) {
    // TODO so none of this works because the only information thats serialized
    // for players is the stuff in PlayerInfo
    //
    // If you want this to work then you have to add it there
    if (!check_name(entity, strings::entity::REMOTE_PLAYER)) return;
    if (entity.id != SystemManager::get().firstPlayerID) return;

    int y_pos = 0;

    auto _draw_text = [&](const std::string& str) mutable {
        float y = 200.f + y_pos;
        DrawTextEx(Preload::get().font, str.c_str(), vec2{5, y}, 20, 0, WHITE);
        y_pos += 15;
    };

    raylib::DrawRectangle(5, 200, 175, 75, (Color){50, 50, 50, 200});

    _draw_text("PlayerInfo:");
    _draw_text(fmt::format("id: {} position: {}", entity.id,
                           entity.get<Transform>().pos()));
    _draw_text(
        fmt::format("holding furniture?: {}",
                    entity.get<CanHoldFurniture>().is_holding_furniture()));
    _draw_text(fmt::format("holding item?: {}",
                           entity.get<CanHoldItem>().is_holding_item()));
}

}  // namespace ui
}  // namespace system_manager
