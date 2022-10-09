#pragma once

#include "entity.h"
#include "external_include.h"
#include "globals.h"
#include "raylib.h"

struct Cam {
    Camera3D camera;
    float target_distance = 0.0f;
    float free_distance_min_clamp = 10.0f;
    float free_distance_max_clamp = 20.0f;
    float free_angle_min = (-70.0f) * DEG2RAD;
    float free_angle_max = (-30.0f) * DEG2RAD;
    float scroll_sensitivity = 1.0f;
    vec3 angle;

    void updateTargetAngle(float dx, float dy, float dz) {
        // Camera angle calculation
        angle.x = atan2f(dx, dz);
        // Camera angle in plane XZ (0 aligned with Z, move positive CCW)
        angle.y = atan2f(dy, sqrtf(dx * dx + dz * dz));
    }

    void updateTargetDistance() {
        float dx = camera.target.x - camera.position.x;
        float dy = camera.target.y - camera.position.y;
        float dz = camera.target.z - camera.position.z;

        target_distance =
            sqrtf(dx * dx + dy * dy + dz * dz);  // Distance to target
    }

    void updateTargetDistanceAndAngle() {
        float dx = camera.target.x - camera.position.x;
        float dy = camera.target.y - camera.position.y;
        float dz = camera.target.z - camera.position.z;

        target_distance =
            sqrtf(dx * dx + dy * dy + dz * dz);  // Distance to target
        updateTargetAngle(dx, dy, dz);
    }

    Cam() {
        auto player = GLOBALS.get<Player>("player");

        this->camera = {};
        this->camera.position = (vec3){0.0f, 10.0f, 10.0f};
        this->camera.target = (vec3){player.position};
        this->camera.up = (vec3){0.0f, 1.0f, 0.0f};
        this->camera.fovy = 45.0f;
        this->camera.projection = CAMERA_PERSPECTIVE;

        updateTargetDistanceAndAngle();

        SetCameraMode(this->camera, CAMERA_CUSTOM);
    }

    void updateToTarget(Player player) {
        camera.target = (vec3){player.position};
    }

    void updateCamera() {
        auto mouseWheelMove = GetMouseWheelMove();
        
        // Camera zoom
        if (mouseWheelMove < 0) {
            target_distance -= (mouseWheelMove * scroll_sensitivity);
        } else if (mouseWheelMove > 0) {
            target_distance += (-mouseWheelMove * scroll_sensitivity);
        }

        if (target_distance < free_distance_min_clamp)
            target_distance = free_distance_min_clamp;
        if (target_distance > free_distance_max_clamp)
            target_distance = free_distance_max_clamp;

        // Update camera position with changes
        camera.position.x =
            -sinf(angle.x) * target_distance * cosf(angle.y) + camera.target.x;
        camera.position.y = -sinf(angle.y) * target_distance + camera.target.y;
        camera.position.z =
            -cosf(angle.x) * target_distance * cosf(angle.y) + camera.target.z;

        // Update angle is there is a mouse delta
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            auto mouseDelta = GetMouseDelta();

            angle.x += mouseDelta.x / 100.0;
            angle.y -= mouseDelta.y / 100.0;

            if (angle.y < free_angle_min)
                angle.y = free_angle_min;
            if (angle.y > free_angle_max)
                angle.y = free_angle_max;
        }
    }

    void debugCamera() {
        std::cout << "Cam(" << this << "):" << std::endl;
        std::cout << "\tGetMouseWheelMove:" << GetMouseWheelMove() << std::endl;
        std::cout << "\tposition:(" << camera.position.x << ", "
                  << camera.position.y << ", " << camera.position.z << ")"
                  << std::endl;
        std::cout << "\target:(" << camera.target.x << ", " << camera.target.y
                  << ", " << camera.target.z << ")" << std::endl;
        std::cout << "\tup:(" << camera.up.x << ", " << camera.up.y << ", "
                  << camera.up.z << ")" << std::endl;
        std::cout << "\tfovy:" << camera.fovy << std::endl;
        std::cout << "\tprojection:" << camera.projection << std::endl;
        std::cout << "\ttarget_distance:" << target_distance << std::endl
                  << std::endl;
    }

    Camera3D get() { return this->camera; }
    Camera3D* get_ptr() { return &(this->camera); }
};
