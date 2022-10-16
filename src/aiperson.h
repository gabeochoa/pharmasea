
#pragma once

#include "astar.h"
#include "entityhelper.h"
#include "external_include.h"
//
#include "globals.h"
#include "person.h"
#include "targetcube.h"
#include "ui_color.h"

struct AIPerson : public Person {
    std::optional<vec2> target;
    std::optional<std::deque<vec2>> path;
    std::optional<vec2> local_target;

    AIPerson(vec3 p, Color face_color_in, Color base_color_in)
        : Person(p, face_color_in, base_color_in) {}
    AIPerson(vec2 p, Color face_color_in, Color base_color_in)
        : Person(p, face_color_in, base_color_in) {}
    AIPerson(vec3 p, Color c) : Person(p, c) {}
    AIPerson(vec2 p, Color c) : Person(p, c) {}

    virtual float base_speed() override { return 10.f; }

    virtual void render() const override {
        Person::render();

        const float box_size = TILESIZE / 10.f;
        if (this->path.has_value()) {
            for (auto location : this->path.value()) {
                DrawCube(vec::to3(location), box_size, box_size, box_size,
                         BLUE);
            }
        }
    }

    void random_target() {
        // TODO add cooldown so that not all time is spent here
        int max_tries = 10;
        int range = 20;
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
    }

    void target_cube_target() {
        std::shared_ptr<TargetCube> closest_target =
            EntityHelper::getClosestMatchingEntity<TargetCube>(
                vec::to2(this->position), TILESIZE * 100.f,
                [](auto&&) { return true; });

        if (!closest_target) return;

        auto snap_near_cube = closest_target->tile_infront(0);
        if (EntityHelper::isWalkable(snap_near_cube)) {
            this->target = snap_near_cube;
        }
    }

    virtual void ensure_target() {
        if (target.has_value()) {
            return;
        }
        this->random_target();
        // this->target_cube_target();
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
        // std::cout << "target" << this->target.value() << std::endl;
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
        float speed = this->base_speed() * dt;

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
        float speed = this->base_speed() * dt;

        auto new_pos_z = this->raw_position;
        if (tar.y > this->raw_position.z) new_pos_z.z += speed;
        if (tar.y < this->raw_position.z) new_pos_z.z -= speed;
        return new_pos_z;
    }

    int path_length() {
        if (!this->path.has_value()) return 0;
        return (int) this->path.value().size();
    }

    void reset_local_target() {
        if (this->local_target.has_value() &&
            vec::to2(this->position) == this->local_target.value()) {
            this->local_target.reset();
        }
    }

    virtual void reset_to_find_new_target() {
        if (this->path_length() == 0) {
            this->target.reset();
            this->path.reset();
            this->local_target.reset();
        }
    }

    virtual void update(float dt) override {
        Person::update(dt);

        this->ensure_target();
        this->ensure_path();
        this->ensure_local_target();
        reset_to_find_new_target();
        reset_local_target();
    }

    virtual bool is_snappable() override { return true; }
};
