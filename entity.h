
#pragma once

#include "astar.h"
#include "external_include.h"
#include "globals.h"
#include "random.h"
#include "raylib.h"
#include "vec_util.h"

BoundingBox get_bounds(vec3 position) {
    const float half_tile = TILESIZE / 2.f;
    return {(vec3){
                position.x - half_tile,
                position.y - half_tile,
                position.z - half_tile,
            },
            (vec3){
                position.x + half_tile,
                position.y + half_tile,
                position.z + half_tile,
            }};
}

static std::atomic_int ENTITY_ID_GEN = 0;
struct Entity {
    int id;
    vec3 position;
    Color color;
    bool cleanup = false;

    Entity(vec3 p, Color c) : id(ENTITY_ID_GEN++), position(p), color(c) {}
    virtual ~Entity() {}

    virtual BoundingBox bounds() const { return get_bounds(this->position); }

    virtual bool collides(BoundingBox b) const {
        return CheckCollisionBoxes(this->bounds(), b);
    }

    virtual void render() const {
        DrawCube(position, TILESIZE, TILESIZE, TILESIZE, this->color);
        DrawBoundingBox(this->bounds(), MAROON);
    }

    vec3 snap_position() { return vec::snap(this->position); }

    virtual void update(float) {}
};
static std::vector<std::shared_ptr<Entity>> entities_DO_NOT_USE;

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

struct Person : public Entity {
    Person(vec3 p, Color c) : Entity(p, c) {}

    virtual vec3 update_xaxis_position(float dt) = 0;
    virtual vec3 update_zaxis_position(float dt) = 0;

    virtual void update(float dt) override {
        auto new_pos_x = this->update_xaxis_position(dt);
        auto new_pos_z = this->update_zaxis_position(dt);

        auto new_bounds_x = get_bounds(new_pos_x);  // horizontal check
        auto new_bounds_y = get_bounds(new_pos_z);  // vertical check

        bool would_collide_x = false;
        bool would_collide_y = false;
        EntityHelper::forEachEntity([&](auto entity) {
            if (this->id == entity->id) {
                return EntityHelper::ForEachFlow::Continue;
            }
            if (CheckCollisionBoxes(new_bounds_x, entity->bounds())) {
                would_collide_x = true;
            }
            if (CheckCollisionBoxes(new_bounds_y, entity->bounds())) {
                would_collide_y = true;
            }
            if (would_collide_x && would_collide_y) {
                return EntityHelper::ForEachFlow::Break;
            }
            return EntityHelper::ForEachFlow::None;
        });

        if (!would_collide_x) {
            this->position.x = new_pos_x.x;
        }
        if (!would_collide_y) {
            this->position.z = new_pos_z.z;
        }
    }
};

struct Player : public Person {
    Player() : Person({}, {0, 255, 0, 255}) {}

    virtual vec3 update_xaxis_position(float dt) override {
        float speed = 10.0f * dt;
        auto new_pos_x = this->position;
        if (IsKeyDown(KEY_D)) new_pos_x.x += speed;
        if (IsKeyDown(KEY_A)) new_pos_x.x -= speed;
        return new_pos_x;
    }

    virtual vec3 update_zaxis_position(float dt) override {
        float speed = 10.0f * dt;
        auto new_pos_z = this->position;
        if (IsKeyDown(KEY_W)) new_pos_z.z -= speed;
        if (IsKeyDown(KEY_S)) new_pos_z.z += speed;
        return new_pos_z;
    }
};

struct Cube : public Entity {
    Cube(vec3 p, Color c) : Entity(p, c) {}
};

struct AIPerson : public Person {
    std::optional<vec2> target;
    std::optional<std::vector<vec2>> path;
    std::optional<vec2> local_target;
    float base_speed = 10.f;

    AIPerson(vec3 p, Color c) : Person(p, c) {}

    void ensure_target() {
        if (target.has_value()) {
            return;
        }
        int range = 10;
        this->target =
            (vec2){1.f * randIn(-range, range), 1.f * randIn(-range, range)};
        std::cout << this->target.value().x << "," << this->target.value().y
                  << std::endl;
    }

    void ensure_path() {
        // no active target
        if (!target.has_value()) {
            return;
        }
        this->path =
            astar::find_path({this->snap_position().x, this->snap_position().z},
                             this->target.value(), [](const vec2& p) {
                                 if (abs(p.x) > 10.f) return false;
                                 if (abs(p.y) > 10.f) return false;
                                 return true;
                             });
        // std::cout << "path " << this->path.value().size() << std::endl;
    }

    void ensure_local_target() {
        // already have one
        if (this->local_target.has_value()) {
            return;
        }
        // no active path
        if (!this->path.has_value()) {
            return;
        }
        if (this->path.value().empty()) {
            return;
        }
        this->local_target = this->path.value()[0];
    }

    virtual vec3 update_xaxis_position(float dt) override {
        if (!this->local_target.has_value()) {
            return this->position;
        }
        vec2 tar = this->local_target.value();
        float speed = this->base_speed * dt;

        auto new_pos_x = this->position;
        if (tar.x > this->position.x) new_pos_x.x += speed;
        if (tar.x < this->position.x) new_pos_x.x -= speed;
        return new_pos_x;
    }

    virtual vec3 update_zaxis_position(float dt) override {
        if (!this->local_target.has_value()) {
            return this->position;
        }
        vec2 tar = this->local_target.value();
        float speed = this->base_speed * dt;

        auto new_pos_z = this->position;
        if (tar.y > this->position.z) new_pos_z.z += speed;
        if (tar.y < this->position.z) new_pos_z.z -= speed;
        return new_pos_z;
    }

    virtual void update(float dt) override {
        this->ensure_target();
        this->ensure_path();
        this->ensure_local_target();

        if (IsKeyReleased(KEY_P)) {
            std::cout << this->position.x << "," << this->position.z
                      << std::endl;
            this->target.reset();
            this->path.reset();
            this->local_target.reset();
        }
        // then handle the normal position stuff
        Person::update(dt);
    }
};
