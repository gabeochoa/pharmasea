#pragma once

#include "globals.h"
#include "raylib.h"
#include "rlgl.h"
//
#include "external_include.h"
#include "vec_util.h"

static void DrawLineStrip2Din3D(const std::vector<vec2>& points, Color color) {
    std::optional<vec3> point;
    for (auto p : points) {
        auto p3 = vec::to3(p);
        p3.x -= TILESIZE / 2;
        p3.y -= TILESIZE / 2;
        p3.z -= TILESIZE / 2;
        if (point.has_value()) {
            DrawLine3D(point.value(), p3, color);
        }
        point = p3;
    }
}

static void DrawCubeCustom(Vector3 position, float width, float height,
                           float length, float front_facing_angle,
                           Color face_color, Color base_color) {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    rlPushMatrix();

    // NOTE: Be careful! Function order matters (rotate -> scale -> translate)
    rlTranslatef(position.x, position.y, position.z);
    rlRotatef(front_facing_angle, 0, 1, 0);
    // rlScalef(2.0f, 2.0f, 2.0f);

    rlBegin(RL_TRIANGLES);
    rlColor4ub(face_color.r, face_color.g, face_color.b, face_color.a);

    // Front Face -----------------------------------------------------
    rlVertex3f(x - width / 2, y - height / 2, z + length / 2);  // Bottom Left
    rlVertex3f(x + width / 2, y - height / 2, z + length / 2);  // Bottom Right
    rlVertex3f(x - width / 2, y + height / 2, z + length / 2);  // Top Left

    rlVertex3f(x + width / 2, y + height / 2, z + length / 2);  // Top Right
    rlVertex3f(x - width / 2, y + height / 2, z + length / 2);  // Top Left
    rlVertex3f(x + width / 2, y - height / 2, z + length / 2);  // Bottom Right

    rlColor4ub(base_color.r, base_color.g, base_color.b, base_color.a);
    // Back Face ------------------------------------------------------
    rlVertex3f(x - width / 2, y - height / 2, z - length / 2);  // Bottom Left
    rlVertex3f(x - width / 2, y + height / 2, z - length / 2);  // Top Left
    rlVertex3f(x + width / 2, y - height / 2, z - length / 2);  // Bottom Right

    rlVertex3f(x + width / 2, y + height / 2, z - length / 2);  // Top Right
    rlVertex3f(x + width / 2, y - height / 2, z - length / 2);  // Bottom Right
    rlVertex3f(x - width / 2, y + height / 2, z - length / 2);  // Top Left

    // Top Face -------------------------------------------------------
    rlVertex3f(x - width / 2, y + height / 2, z - length / 2);  // Top Left
    rlVertex3f(x - width / 2, y + height / 2, z + length / 2);  // Bottom Left
    rlVertex3f(x + width / 2, y + height / 2, z + length / 2);  // Bottom Right

    rlVertex3f(x + width / 2, y + height / 2, z - length / 2);  // Top Right
    rlVertex3f(x - width / 2, y + height / 2, z - length / 2);  // Top Left
    rlVertex3f(x + width / 2, y + height / 2, z + length / 2);  // Bottom Right

    // Bottom Face ----------------------------------------------------
    rlVertex3f(x - width / 2, y - height / 2, z - length / 2);  // Top Left
    rlVertex3f(x + width / 2, y - height / 2, z + length / 2);  // Bottom Right
    rlVertex3f(x - width / 2, y - height / 2, z + length / 2);  // Bottom Left

    rlVertex3f(x + width / 2, y - height / 2, z - length / 2);  // Top Right
    rlVertex3f(x + width / 2, y - height / 2, z + length / 2);  // Bottom Right
    rlVertex3f(x - width / 2, y - height / 2, z - length / 2);  // Top Left

    // Right face -----------------------------------------------------
    rlVertex3f(x + width / 2, y - height / 2, z - length / 2);  // Bottom Right
    rlVertex3f(x + width / 2, y + height / 2, z - length / 2);  // Top Right
    rlVertex3f(x + width / 2, y + height / 2, z + length / 2);  // Top Left

    rlVertex3f(x + width / 2, y - height / 2, z + length / 2);  // Bottom Left
    rlVertex3f(x + width / 2, y - height / 2, z - length / 2);  // Bottom Right
    rlVertex3f(x + width / 2, y + height / 2, z + length / 2);  // Top Left

    // Left Face ------------------------------------------------------
    rlVertex3f(x - width / 2, y - height / 2, z - length / 2);  // Bottom Right
    rlVertex3f(x - width / 2, y + height / 2, z + length / 2);  // Top Left
    rlVertex3f(x - width / 2, y + height / 2, z - length / 2);  // Top Right

    rlVertex3f(x - width / 2, y - height / 2, z + length / 2);  // Bottom Left
    rlVertex3f(x - width / 2, y + height / 2, z + length / 2);  // Top Left
    rlVertex3f(x - width / 2, y - height / 2, z - length / 2);  // Bottom Right
    rlEnd();
    rlPopMatrix();
}
