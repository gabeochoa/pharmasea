#pragma once

#include "external_include.h"
#include "raylib.h"

struct Cam {
    Camera3D camera;

    Cam() {
        this->camera = {0};
        this->camera.position = (vec3){0.0f, 10.0f, 10.0f};
        this->camera.target = (vec3){0.0f, 0.0f, 0.0f};
        this->camera.up = (vec3){0.0f, 1.0f, 0.0f};
        this->camera.fovy = 45.0f;
        this->camera.projection = CAMERA_PERSPECTIVE;

    SetCameraMode(this->camera, CAMERA_FREE);
    }

    Camera3D get() { return this->camera; }
    Camera3D* get_ptr() { return &(this->camera); }
};
