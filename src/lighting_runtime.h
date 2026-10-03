#pragma once

#include "engine/graphics.h"

// Lighting runtime data + uniform updates for the scene lighting shader.
void update_lighting_shader(raylib::Shader& shader,
                            const raylib::Camera3D& cam);
