#pragma once

#include "external_include.h"
//
#include "engine/log.h"
#include "globals.h"
#include "raylib.h"

struct GameCam {
    raylib::Camera3D camera;
    float target_distance = 0.0f;
    float free_distance_min_clamp = 10.0f;
    float free_distance_max_clamp = 20.0f;
    float free_angle_min = (-89.0f) * DEG2RAD;
    float free_angle_max = (0.0f) * DEG2RAD;
    float scroll_sensitivity = 1.0f;
    float default_angle_y = -45.f * DEG2RAD;
    vec2 angle = {0.0, 0.0};

    void angleMinMaxClamp() {
        if (angle.y < free_angle_min) angle.y = free_angle_min;
        if (angle.y > free_angle_max) angle.y = free_angle_max;
    }

    void updateTargetAngle(float dx, float dy, float dz) {
        // Camera angle calculation
        angle.x = atan2f(dx, dz);
        // Camera angle in plane XZ (0 aligned with Z, move positive CCW)
        angle.y = atan2f(dy, sqrtf(dx * dx + dz * dz));
        angleMinMaxClamp();
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

    GameCam() {
        this->camera = {};
        this->camera.position = (vec3){0.0f, 10.0f, 10.0f};
        this->camera.target = (vec3){0, 0, 0};
        this->camera.up = (vec3){0.0f, 1.0f, 0.0f};
        this->camera.fovy = 45.0f;
        this->camera.projection = raylib::CAMERA_PERSPECTIVE;

        updateTargetDistanceAndAngle();
        angle.y = default_angle_y;

        raylib::SetCameraMode(this->camera, raylib::CAMERA_CUSTOM);
    }

    void updateToTarget(const vec3& position) { camera.target = position; }

    void updateCamera() {
        auto mouseWheelMove = ext::get_mouse_wheel_move();

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
        if (IsMouseButtonDown(raylib::MOUSE_BUTTON_LEFT)) {
            auto mouseDelta = raylib::GetMouseDelta();

            angle.x -= mouseDelta.x / 100.0f;
            angle.y -= mouseDelta.y / 100.0f;

            angleMinMaxClamp();
        }

        float right_x_axis =
            GetGamepadAxisMovement(0, raylib::GAMEPAD_AXIS_RIGHT_X);
        float right_y_axis =
            GetGamepadAxisMovement(0, raylib::GAMEPAD_AXIS_RIGHT_Y);
        if (abs(right_x_axis) > EPSILON || abs(right_y_axis) > EPSILON) {
            angle.x -= right_x_axis / 50.f;
            angle.y -= right_y_axis / 50.f;
            angleMinMaxClamp();
        }
    }

    void debugCamera() { log_trace("{}", *this); }

    raylib::Camera3D get() { return this->camera; }
    raylib::Camera3D* get_ptr() { return &(this->camera); }
};

inline std::ostream& operator<<(std::ostream& os, const GameCam& gc) {
    os << "Cam(" << &gc << "):" << std::endl;
    os << "\tGetMouseWheelMove:" << ext::get_mouse_wheel_move() << std::endl;
    os << "\tposition:(" << gc.camera.position.x << ", " << gc.camera.position.y
       << ", " << gc.camera.position.z << ")" << std::endl;
    os << "\target:(" << gc.camera.target.x << ", " << gc.camera.target.y
       << ", " << gc.camera.target.z << ")" << std::endl;
    os << "\tup:(" << gc.camera.up.x << ", " << gc.camera.up.y << ", "
       << gc.camera.up.z << ")" << std::endl;
    os << "\tfovy:" << gc.camera.fovy << std::endl;
    os << "\tprojection:" << gc.camera.projection << std::endl;
    os << "\ttarget_distance:" << gc.target_distance << std::endl << std::endl;
    return os;
}

struct MenuCam {
    raylib::Camera3D camera;

    MenuCam() {
        this->camera = {};
        this->camera.position = (vec3){0.0f, 10.0f, 10.0f};
        this->camera.target = (vec3){0.f};
        this->camera.up = (vec3){0.0f, 1.0f, 0.0f};
        this->camera.fovy = 45.0f;
        this->camera.projection = raylib::CAMERA_PERSPECTIVE;

        SetCameraMode(this->camera, raylib::CAMERA_FREE);
    }

    void updateCamera() {}

    raylib::Camera3D get() { return this->camera; }
    raylib::Camera3D* get_ptr() { return &(this->camera); }
};
