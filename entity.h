
#pragma once

#include "external_include.h"
#include "globals.h"
#include "raylib.h"


struct Entity {
    vec3 position;
    Color color;
    bool cleanup = false;

    Entity(vec3 p, Color c) : position(p), color(c) {}
    virtual ~Entity(){}

    virtual BoundingBox bounds() const {
        const float half_tile = TILESIZE / 2.f;
        return {
            (vec3){
                position.x - half_tile, 
                position.y - half_tile, 
                position.z - half_tile, 
            },
            (vec3){
                position.x + half_tile, 
                position.y + half_tile, 
                position.z + half_tile, 
            }
        };
    }

    virtual bool collides(BoundingBox b) const {
        return CheckCollisionBoxes(this->bounds(), b);
    }

    virtual void render() const {
        DrawCube(position, TILESIZE, TILESIZE, TILESIZE, this->color);
        DrawBoundingBox(this->bounds(), MAROON);
    }

    virtual void update(float) {}
};

struct Player : public Entity {
    Player() : Entity({0}, {0, 255, 0, 255}) {}

    virtual void update(float dt) {
        float speed = 10.0f * dt;
        if (IsKeyDown(KEY_D)) this->position.x += speed;
        if (IsKeyDown(KEY_A)) this->position.x -= speed;
        if (IsKeyDown(KEY_W)) this->position.z -= speed;
        if (IsKeyDown(KEY_S)) this->position.z += speed;
    }

};

struct Cube : public Entity {
    Cube(vec3 p, Color c) : Entity(p, c) {}
};

static std::vector<std::shared_ptr<Entity>>
    entities_DO_NOT_USE;

struct EntityHelper {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    static void addEntity(std::shared_ptr<Entity> e) {
        entities_DO_NOT_USE.push_back(e);
    }
    static void cleanup() {
        // Cleanup entities marked cleanup
        auto it = entities_DO_NOT_USE.begin();
        while (it != entities_DO_NOT_USE.end()) {
            if ((*it)->cleanup) {
                entities_DO_NOT_USE.erase(it);
                continue;
            }
            it++;
        }
    }

    enum ForEachFlow {
        None = 0,
        Continue = 1,
        Break = 2,
    };

    static void forEachEntity(
        std::function<ForEachFlow(std::shared_ptr<Entity>)> cb) {
        for (auto e : entities_DO_NOT_USE) {
            if (!e) continue;
            auto fef = cb(e);
            if (fef == 1) continue;
            if (fef == 2) break;
        }
    }
#pragma clang diagnostic pop
};
