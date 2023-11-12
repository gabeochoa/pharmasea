

#pragma once

#include "../camera.h"
#include "../components/can_be_highlighted.h"
#include "../components/can_order_drink.h"
#include "../components/has_name.h"
#include "../components/has_speech_bubble.h"
#include "../components/has_work.h"
#include "../components/is_drink.h"
#include "../components/is_pnumatic_pipe.h"
#include "../components/is_trigger_area.h"
#include "../components/model_renderer.h"
#include "../components/shows_progress_bar.h"
#include "../components/simple_colored_box_renderer.h"
#include "../components/transform.h"
#include "../components/uses_character_model.h"
#include "../engine/log.h"
#include "../engine/time.h"
#include "job_system.h"

namespace system_manager {
namespace render_manager {

void draw_valid_colored_box(const Transform& transform,
                            const SimpleColoredBoxRenderer& renderer,
                            bool is_highlighted);
void update_character_model_from_index(Entity& entity, float);
bool render_simple_highlighted(const Entity& entity, float);
bool render_simple_normal(const Entity& entity, float);
bool render_bounding_box(const Entity& entity, float);
void render_debug_subtype(const Entity& entity, float);
void render_debug_drink_info(const Entity& entity, float);
void render_debug_filter_info(const Entity& entity, float);
bool render_debug(const Entity& entity, float dt);
bool render_model_highlighted(const Entity& entity, float);
bool render_model_normal(const Entity& entity, float);
void render_trigger_area(const Entity& entity, float dt);
void render_speech_bubble(const Entity& entity, float);
void render_normal(const Entity& entity, float dt);
void render_floating_name(const Entity& entity, float);
void render_progress_bar(const Entity& entity, float);
void render_waiting_queue(const Entity& entity, float);
void render_walkable_spots(float);

void render_trash_marker(const Entity& entity);
void render_dollarsign_marker(const Entity& entity);

void render(const Entity&, float, bool);

}  // namespace render_manager

}  // namespace system_manager
