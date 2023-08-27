#pragma once

#include "external_include.h"
//
#include "engine/log.h"
#include "engine/ui/color.h"
#include "globals.h"
#include "raylib.h"
#include "rlgl.h"
//
#include "text_util.h"
#include "vec_util.h"

namespace raylib {

static void DrawPctFilledCircle(const vec2 position, float radius,
                                Color backgroundColor, Color foregroundColor,
                                float pct_filled, float startAngle = 180) {
    const float endAngle = startAngle + (360 * pct_filled);
    const int segments = 40;

    raylib::DrawCircle((int) position.x, (int) position.y, radius,
                       backgroundColor);
    raylib::DrawCircleSector(position, radius, startAngle, endAngle, segments,
                             foregroundColor);  // Draw a piece of a circle
}

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

static void DrawRect2Din3D(Vector3 position, float width, float height,
                           Color color = RED) {
    // TODO :NONE: this code doesnt work yet,
    // TODO :INFRA: likely want to add Z rotation so it follows camera (but we
    // cannot include "cam.h" in here
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    rlPushMatrix();

    // NOTE: Be careful! Function order matters (rotate -> scale -> translate)
    rlTranslatef(position.x, position.y + TILESIZE, position.z);
    // rlRotatef(front_facing_angle, 0, 1, 0);
    // rlScalef(2.0f, 2.0f, 2.0f);

    float length = width;

    rlBegin(RL_TRIANGLES);
    rlColor4ub(color.r, color.g, color.b, color.a);

    // Front Face -----------------------------------------------------
    rlVertex3f(x - width / 2, y - height / 2, z + length / 2);  // Bottom Left
    rlVertex3f(x + width / 2, y - height / 2, z + length / 2);  // Bottom Right
    rlVertex3f(x - width / 2, y + height / 2, z + length / 2);  // Top Left

    rlVertex3f(x + width / 2, y + height / 2, z + length / 2);  // Top Right
    rlVertex3f(x - width / 2, y + height / 2, z + length / 2);  // Top Left
    rlVertex3f(x + width / 2, y - height / 2, z + length / 2);  // Bottom Right

    // Back Face ------------------------------------------------------
    rlVertex3f(x - width / 2, y - height / 2, z - length / 2);  // Bottom Left
    rlVertex3f(x - width / 2, y + height / 2, z - length / 2);  // Top Left
    rlVertex3f(x + width / 2, y - height / 2, z - length / 2);  // Bottom Right

    rlVertex3f(x + width / 2, y + height / 2, z - length / 2);  // Top Right
    rlVertex3f(x + width / 2, y - height / 2, z - length / 2);  // Bottom Right
    rlVertex3f(x - width / 2, y + height / 2, z - length / 2);  // Top Left

    rlEnd();
    rlPopMatrix();
}

static void DrawCubeCustom(Vector3 position, float width, float height,
                           float length, float front_facing_angle,
                           Color face_color, Color base_color) {
    // This logif exists because i had a problem where all furniture was
    // size(0,0,0) and took me a while to figure it out.
    log_ifx((width == 0 || height == 0 || length == 0), LogLevel::LOG_WARN,
            "Trying to render entity that has size ({}, {}, {})", width, height,
            length);

    if (width == 0 || height == 0 || length == 0) {
        log_error("");
    }

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

static void DrawFloatingText(const vec3& position, Font font, const char* text,
                             int size = 96, Color color = BLACK) {
    rlPushMatrix();
    rlTranslatef(    //
        position.x,  //
        position.y,  //
        position.z   //
    );
    rlRotatef(90.0f, 1.0f, 0.0f, 0.0f);

    rlTranslatef(          //
        -0.5f * TILESIZE,  //
        0.f,               //
        -1.05f * TILESIZE  // this is Y
    );

    DrawText3D(      //
        font, text,  //
        {0.f},       //
        size,        // font size
        4,           // font spacing
        4,           // line spacing
        true,        // backface
        color);

    rlPopMatrix();
}

}  // namespace raylib
