#pragma once

#include "external_include.h"
//
#include "engine/log.h"
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

    void angleMinMaxClamp();
    void updateTargetAngle(float dx, float dy, float dz);
    void updateTargetDistance();
    void updateTargetDistanceAndAngle();
    GameCam();
    void updateToTarget(const vec3& position);
    void updateCamera();
    void debugCamera() { log_trace("{}", *this); }
    raylib::Camera3D get() { return this->camera; }
    raylib::Camera3D* get_ptr() { return &(this->camera); }
};

inline std::ostream& operator<<(std::ostream& os, const GameCam& gc) {
    os << "Cam(" << &gc << "):" << std::endl;
    os << "\tGetMouseWheelMove:" << ext::get_mouse_wheel_move() << std::endl;
    os << "\tposition:(" << gc.camera.position.x << ", " << gc.camera.position.y
       << ", " << gc.camera.position.z << ")" << std::endl;
    os << "\ttarget:(" << gc.camera.target.x << ", " << gc.camera.target.y
       << ", " << gc.camera.target.z << ")" << std::endl;
    os << "\tup:(" << gc.camera.up.x << ", " << gc.camera.up.y << ", "
       << gc.camera.up.z << ")" << std::endl;
    os << "\tfovy:" << gc.camera.fovy << std::endl;
    os << "\tprojection:" << gc.camera.projection << std::endl;
    os << "\ttarget_distance:" << gc.target_distance << std::endl << std::endl;
    os << "\tangle:" << gc.angle << std::endl << std::endl;
    return os;
}
