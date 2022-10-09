
#pragma once

#include "entity.h"
#include "external_include.h"

struct Person : public Entity {
    Person(vec3 p, Color c) : Entity(p, c) {}
    Person(vec2 p, Color c) : Entity(p, c) {}

    virtual vec3 update_xaxis_position(float dt) = 0;
    virtual vec3 update_zaxis_position(float dt) = 0;

    virtual vec3 size() const override {
        return (vec3){TILESIZE * 0.8, TILESIZE * 0.8, TILESIZE * 0.8};
    }

    virtual void update(float dt) override {
        // std::cout << this->raw_position << ";; " << this->position <<
        // std::endl;
        auto new_pos_x = this->update_xaxis_position(dt);
        auto new_pos_z = this->update_zaxis_position(dt);

        auto new_bounds_x =
            get_bounds(new_pos_x, this->size());  // horizontal check
        auto new_bounds_y =
            get_bounds(new_pos_z, this->size());  // vertical check

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
            this->raw_position.x = new_pos_x.x;
        }
        if (!would_collide_y) {
            this->raw_position.z = new_pos_z.z;
        }
        
        Entity::update(dt);
    }
};

struct Player : public Person {
    Player() : Person({0, 0, 0}, {0, 255, 0, 255}) {}
    Player(vec2 location)
        : Person({location.x, 0, location.y}, {0, 255, 0, 255}) {}

    virtual vec3 update_xaxis_position(float dt) override {
        float speed = 10.0f * dt;
        auto new_pos_x = this->raw_position;
        if (IsKeyDown(KEY_D)) new_pos_x.x += speed;
        if (IsKeyDown(KEY_A)) new_pos_x.x -= speed;
        return new_pos_x;
    }

    virtual vec3 update_zaxis_position(float dt) override {
        float speed = 10.0f * dt;
        auto new_pos_z = this->raw_position;
        if (IsKeyDown(KEY_W)) new_pos_z.z -= speed;
        if (IsKeyDown(KEY_S)) new_pos_z.z += speed;
        return new_pos_z;
    }
};

struct Cube : public Entity {
    Cube(vec3 p, Color c) : Entity(p, c) {}
    Cube(vec2 p, Color c) : Entity(p, c) {}
};

struct AIPerson : public Person {
    std::optional<vec2> target;
    std::optional<std::deque<vec2>> path;
    std::optional<vec2> local_target;
    float base_speed = 10.f;

    AIPerson(vec3 p, Color c) : Person(p, c) {}
    AIPerson(vec2 p, Color c) : Person(p, c) {}

    virtual void render() const override {
        Person::render();
        if (!this->path.has_value()) {
            return;
        }
        const float box_size = TILESIZE / 10.f;
        for (auto location : this->path.value()) {
            DrawCube(vec::to3(location), box_size, box_size, box_size, BLUE);
        }
    }

    void ensure_target() {
        if (target.has_value()) {
            return;
        }
        // TODO add cooldown so that not all time is spent here
        int max_tries = 10;
        int range = 10;
        bool walkable = false;
        int i = 0;
        while (!walkable) {
            this->target = (vec2){1.f * randIn(-range, range),
                                  1.f * randIn(-range, range)};
            walkable = EntityHelper::isWalkable(this->target.value());
            i++;
            if (i > max_tries) {
                break;
            }
        }
        // std::cout << this->target.value() << ", " << walkable << std::endl;
    }

    void ensure_path() {
        // no active target
        if (!target.has_value()) {
            return;
        }
        // If we have a path, don't regenerate a new path so soon.
        if (this->path.has_value()) {
            return;
        }
        this->path = astar::find_path(
            {this->position.x, this->position.z}, this->target.value(),
            std::bind(EntityHelper::isWalkable, std::placeholders::_1));
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
        this->local_target = this->path.value().front();
        this->path.value().pop_front();
    }

    virtual vec3 update_xaxis_position(float dt) override {
        if (!this->local_target.has_value()) {
            return this->raw_position;
        }
        vec2 tar = this->local_target.value();
        float speed = this->base_speed * dt;

        auto new_pos_x = this->raw_position;
        if (tar.x > this->raw_position.x) new_pos_x.x += speed;
        if (tar.x < this->raw_position.x) new_pos_x.x -= speed;
        return new_pos_x;
    }

    virtual vec3 update_zaxis_position(float dt) override {
        if (!this->local_target.has_value()) {
            return this->raw_position;
        }
        vec2 tar = this->local_target.value();
        float speed = this->base_speed * dt;

        auto new_pos_z = this->raw_position;
        if (tar.y > this->raw_position.z) new_pos_z.z += speed;
        if (tar.y < this->raw_position.z) new_pos_z.z -= speed;
        return new_pos_z;
    }

    virtual void update(float dt) override {
        this->ensure_target();
        this->ensure_path();
        this->ensure_local_target();

        if (IsKeyReleased(KEY_P)) {
            std::cout << this->raw_position << ";; " << this->position
                      << std::endl;
            this->target.reset();
            this->path.reset();
            this->local_target.reset();
        }
        // then handle the normal position stuff
        Person::update(dt);

        if (this->local_target.has_value() && vec::to2(this->position) == this->local_target.value()) {
            this->local_target.reset();
        }
    }
};
