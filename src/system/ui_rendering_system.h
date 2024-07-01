#pragma once

#include "../engine/ui/ui.h"
#include "../entity.h"

extern ui::UITheme UI_THEME;

namespace system_manager {
namespace ui {

void render_current_register_queue(float dt);
void render_timer(const Entity& entity, float);
void render_networked_players(const Entities&, float dt);
void render_normal(const Entities& entities, float dt);

}  // namespace ui
}  // namespace system_manager
