
#pragma once

#include "entity.h"
#include "external_include.h"
#include "input.h"
#include "keymap.h"

struct Person : public Entity {
    Person(vec3 p, Color face_color_in, Color base_color_in) : Entity(p, face_color_in, base_color_in) {}
    Person(vec2 p, Color face_color_in, Color base_color_in) : Entity(p, face_color_in, base_color_in) {}
    Person(vec3 p, Color c) : Entity(p, c) {}
    Person(vec2 p, Color c) : Entity(p, c) {}

    virtual vec3 update_xaxis_position(float dt) = 0;
    virtual vec3 update_zaxis_position(float dt) = 0;

    vec3 prev_position;

    virtual vec3 size() const override {
        return (vec3){TILESIZE * 0.8, TILESIZE * 0.8, TILESIZE * 0.8};
    }

    virtual void update(float dt) override {
        // std::cout << this->raw_position << ";; " << this->position <<
        // std::endl;

        auto new_pos_x = this->update_xaxis_position(dt);
        auto new_pos_z = this->update_zaxis_position(dt);

        vec3 delta_distance_x = new_pos_x - this->raw_position;
        if (delta_distance_x.x > 0) {
            this->face_direction = FrontFaceDirection::RIGHT;
        }
        else if (delta_distance_x.x < 0) {
            this->face_direction = FrontFaceDirection::LEFT;
        }

        vec3 delta_distance_z = new_pos_z - this->raw_position;
        if (delta_distance_z.z > 0) {
            this->face_direction = FrontFaceDirection::FORWARD;
        }
        else if (delta_distance_z.z < 0) {
            this->face_direction = FrontFaceDirection::BACK;
        }

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
            if (!entity->is_collidable()) {
                return EntityHelper::ForEachFlow::Continue;
            }
            if (!this->is_collidable()) {
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
    Player(vec3 p, Color face_color_in, Color base_color_in) : Person(p, face_color_in, base_color_in) {}
    Player(vec2 p, Color face_color_in, Color base_color_in) : Person(p, face_color_in, base_color_in) {}
    Player() : Person({ 0, 0, 0 }, { 0, 255, 0, 255 }, {255, 0, 0, 255}) {}
    Player(vec2 location)
        : Person({location.x, 0, location.y}, {0, 255, 0, 255}, { 255, 0, 0, 255 }) {}

    virtual vec3 update_xaxis_position(float dt) override {
        float speed = 10.0f * dt;
        auto new_pos_x = this->raw_position;
        bool left = KeyMap::is_event(Menu::State::Game, "Player Left");
        bool right = KeyMap::is_event(Menu::State::Game, "Player Right");
        if (left) { 
            new_pos_x.x -= speed;
        }
        if (right) {
            new_pos_x.x += speed;
        }
        return new_pos_x;
    }

    virtual vec3 update_zaxis_position(float dt) override {
        float speed = 10.0f * dt;
        auto new_pos_z = this->raw_position;
        bool up = KeyMap::is_event(Menu::State::Game, "Player Forward");
        bool down = KeyMap::is_event(Menu::State::Game, "Player Back");
        if (up) {
            new_pos_z.z -= speed;
        }
        if (down) {
            new_pos_z.z += speed;
        }
        return new_pos_z;
    }

    virtual void update(float dt) override {
        Person::update(dt);
        grab_or_drop_item();
    }

    virtual void grab_or_drop_item() {
        bool pickup = KeyMap::is_event_once_DO_NOT_USE(Menu::State::Game, "Player Pickup");
        if (pickup) {
            if (held_item != nullptr) {
                held_item = nullptr;
            } else {
                std::shared_ptr<Item> closest_item = nullptr;
                float best_distance = std::numeric_limits<float>::max();
                for (auto item : items_DO_NOT_USE) {
                    if (item->collides(
                            get_bounds(this->position, this->size() * 4.0f))) {
                        auto current_distance = vec::distance(
                            vec::to2(item->position), vec::to2(this->position));
                        if (current_distance < best_distance) {
                            best_distance = current_distance;
                            closest_item = item;
                        }
                    }
                }
                // std::cout << "Grabbing item:" << closest_item << std::endl;
                if (closest_item != nullptr) {
                    this->held_item = closest_item;
                }
            }
        }
    }
};

struct TargetCube : public Person {

    TargetCube(vec3 p, Color face_color_in, Color base_color_in) : Person(p, face_color_in, base_color_in) {}
    TargetCube(vec2 p, Color face_color_in, Color base_color_in) : Person(p, face_color_in, base_color_in) {}
    TargetCube(vec3 p, Color c) : Person(p, c) {}
    TargetCube(vec2 p, Color c) : Person(p, c) {}

    virtual vec3 update_xaxis_position(float dt) override {
        float speed = 10.0f * dt;
        auto new_pos_x = this->raw_position;
        bool left = KeyMap::is_event(Menu::State::Game, "Target Left");
        bool right = KeyMap::is_event(Menu::State::Game, "Target Right");
        if (left) new_pos_x.x -= speed;
        if (right) new_pos_x.x += speed;
        return new_pos_x;
    }

    virtual vec3 update_zaxis_position(float dt) override {
        float speed = 10.0f * dt;
        auto new_pos_z = this->raw_position;
        bool up = KeyMap::is_event(Menu::State::Game, "Target Forward");
        bool down = KeyMap::is_event(Menu::State::Game, "Target Back");
        if (up) new_pos_z.z -= speed;
        if (down) new_pos_z.z += speed;
        return new_pos_z;
    }

    virtual bool is_collidable() override { return false; }
};

struct AIPerson : public Person {
    std::optional<vec2> target;
    std::optional<std::deque<vec2>> path;
    std::optional<vec2> local_target;
    float base_speed = 10.f;

    AIPerson(vec3 p, Color face_color_in, Color base_color_in) : Person(p, face_color_in, base_color_in) {}
    AIPerson(vec2 p, Color face_color_in, Color base_color_in) : Person(p, face_color_in, base_color_in) {}
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

    void random_target() {
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
    }

    void target_cube_target() {
        auto snap_near_cube =
            vec::to2(GLOBALS.get<TargetCube>("targetcube").snap_position());
        snap_near_cube.x += TILESIZE;
        snap_near_cube.y += TILESIZE;
        if (EntityHelper::isWalkable(snap_near_cube)) {
            this->target = snap_near_cube;
        }
    }

    void ensure_target() {
        if (target.has_value()) {
            return;
        }
        // this->random_target();
        this->target_cube_target();
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

    int path_length() {
        if (!this->path.has_value()) return 0;
        return (int)this->path.value().size();
    }

    virtual void update(float dt) override {
        this->ensure_target();
        this->ensure_path();
        this->ensure_local_target();

        if (this->path_length() == 0) {
            // std::cout << this->raw_position << ";; " << this->position
            //           << std::endl;
            this->target.reset();
            this->path.reset();
            this->local_target.reset();
        }
        // then handle the normal position stuff
        Person::update(dt);

        if (this->local_target.has_value() &&
            vec::to2(this->position) == this->local_target.value()) {
            this->local_target.reset();
        }
    }

    virtual bool is_snappable() { return true; }
};
