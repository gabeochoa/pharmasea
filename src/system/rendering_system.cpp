
#include "rendering_system.h"

#include "system_manager.h"

namespace system_manager {
namespace ui {

void render_networked_players(float dt) {
    int y_pos = 0;

    auto _draw_text = [&](const std::string& str) mutable {
        float y = 100.f + y_pos;
        DrawTextEx(Preload::get().font, str.c_str(), vec2{WIN_WF() - 150, y},
                   20, 0, WHITE);
        y_pos += 15;
    };

    const auto _render_single_networked_player =
        [&](std::shared_ptr<Entity> entity, float) {
            _draw_text(entity->get<HasName>().name());
        };

    for (auto& entity : SystemManager::get().oldAll) {
        // TODO again pls make a better way
        if (entity->get<DebugName>().name() != "player") continue;
        _render_single_networked_player(entity, dt);
    }
}

}  // namespace ui
}  // namespace system_manager
