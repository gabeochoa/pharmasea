
#include "camera.h"

#include "engine/settings.h"

void GameCam::angleMinMaxClamp() {
    if (angle.y < free_angle_min) angle.y = free_angle_min;
    if (angle.y > free_angle_max) angle.y = free_angle_max;
}

void GameCam::updateTargetAngle(float dx, float dy, float dz) {
    // Camera angle calculation
    angle.x = atan2f(dx, dz);
    // Camera angle in plane XZ (0 aligned with Z, move positive CCW)
    angle.y = atan2f(dy, sqrtf(dx * dx + dz * dz));
    angleMinMaxClamp();
}

void GameCam::updateTargetDistance() {
    float dx = camera.target.x - camera.position.x;
    float dy = camera.target.y - camera.position.y;
    float dz = camera.target.z - camera.position.z;

    target_distance = sqrtf(dx * dx + dy * dy + dz * dz);  // Distance to target
}

void GameCam::updateTargetDistanceAndAngle() {
    float dx = camera.target.x - camera.position.x;
    float dy = camera.target.y - camera.position.y;
    float dz = camera.target.z - camera.position.z;

    target_distance = sqrtf(dx * dx + dy * dy + dz * dz);  // Distance to target
    updateTargetAngle(dx, dy, dz);
}

GameCam::GameCam() {
    this->camera = {};
    this->camera.position = (vec3) {0.0f, 10.0f, 10.0f};
    this->camera.target = (vec3) {0, 0, 0};
    this->camera.up = (vec3) {0.0f, 1.0f, 0.0f};
    this->camera.fovy = 45.0f;
    this->camera.projection = raylib::CameraProjection::CAMERA_PERSPECTIVE;

    updateTargetDistanceAndAngle();
    angle.y = default_angle_y;

    raylib::UpdateCamera(&(this->camera), raylib::CameraMode::CAMERA_CUSTOM);
}

void GameCam::updateToTarget(const vec3& position) { camera.target = position; }

void GameCam::updateCamera() {
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

    // Return early and skip doing anything
    if (Settings::get().data.snapCameraTo90) {
        angle.x = (float) M_PI;
        angle.y = default_angle_y;
        return;
    }

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
