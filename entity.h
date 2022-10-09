
#pragma once 

#include "external_include.h"


struct Cube {
    vec3 position;
    Color color;

    Cube(vec3 p, Color c) : position(p), color(c) {}

    virtual void render() {
        DrawCube(position, 2.0f, 2.0f, 2.0f, this->color);
        DrawCubeWires(position, 2.0f, 2.0f, 2.0f, MAROON);
    }

    virtual void update(float dt) {}
};

struct Player : public Cube {
    Player() : Cube({0}, {0, 255, 0, 255}) {}

    virtual void update(float dt) {
        float speed = 10.0f * dt;
        if (IsKeyDown(KEY_D)) this->position.x += speed;
        if (IsKeyDown(KEY_A)) this->position.x -= speed;
        if (IsKeyDown(KEY_W)) this->position.z -= speed;
        if (IsKeyDown(KEY_S)) this->position.z += speed;
    }

} player;


