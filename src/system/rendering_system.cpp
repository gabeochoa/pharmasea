
#include "rendering_system.h"

#include "system_manager.h"

namespace system_manager {
namespace ui {

void render_networked_players(const Entities& entities, float dt) {
    int y_pos = 0;

    auto _draw_text = [&](const std::string& str) mutable {
        float y = 200.f + y_pos;
        DrawTextEx(Preload::get().font, str.c_str(), vec2{WIN_WF() - 300, y},
                   40, 0, WHITE);
        y_pos += 30;
    };

    const auto _render_single_networked_player =
        [&](std::shared_ptr<Entity> entity, float) {
            _draw_text(entity->get<HasName>().name());
        };

    for (auto& entity : SystemManager::get().oldAll) {
        // for (auto& entity : entities) {
        // TODO again pls make a better way
        if (entity->get<DebugName>().name() != "player" &&
            // TODO think about this harder
            entity->get<DebugName>().name() != "remote player")
            continue;
        _render_single_networked_player(entity, dt);
    }
}

}  // namespace ui
}  // namespace system_manager
