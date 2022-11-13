
#pragma once

#include "external_include.h"
//
#include "entity.h"
#include "entityhelper.h"
//
#include "keymap.h"

struct Person : public Entity {
   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Entity>{});
    }

   public:
    Person() : Entity() {}
    Person(vec3 p, Color face_color_in, Color base_color_in)
        : Entity(p, face_color_in, base_color_in) {}
    Person(vec2 p, Color face_color_in, Color base_color_in)
        : Entity(p, face_color_in, base_color_in) {}
    Person(vec3 p, Color c) : Entity(p, c) {}
    Person(vec2 p, Color c) : Entity(p, c) {}

    virtual vec3 update_xaxis_position(float dt) = 0;
    virtual vec3 update_zaxis_position(float dt) = 0;

    virtual float base_speed() { return 10.f; }

    virtual vec3 size() const override {
        return (vec3){TILESIZE * 0.8f, TILESIZE * 0.8f, TILESIZE * 0.8f};
    }

    void handle_collision(int facedir_x, vec3 new_pos_x, int facedir_z,
                          vec3 new_pos_z) {
        auto new_bounds_x =
            get_bounds(new_pos_x, this->size());  // horizontal check
        auto new_bounds_y =
            get_bounds(new_pos_z, this->size());  // vertical check

        bool would_collide_x = false;
        bool would_collide_z = false;
        std::weak_ptr<Entity> collided_entity_x;
        std::weak_ptr<Entity> collided_entity_z;
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
                collided_entity_x = entity;
            }
            if (CheckCollisionBoxes(new_bounds_y, entity->bounds())) {
                would_collide_z = true;
                collided_entity_z = entity;
            }
            if (would_collide_x && would_collide_z) {
                return EntityHelper::ForEachFlow::Break;
            }

            return EntityHelper::ForEachFlow::None;
        });

        if (!would_collide_x) {
            this->raw_position.x = new_pos_x.x;
        }
        if (!would_collide_z) {
            this->raw_position.z = new_pos_z.z;
        }

        // This value determines how "far" to impart a push force on the
        // collided entity
        const float directional_push_modifier = 1.0f;

        // Figure out if there's a more graceful way to "jitter" things around
        // each other
        if (would_collide_x || would_collide_z) {
            if (auto entity_x = collided_entity_x.lock()) {
                if (auto person_ptr_x = dynamic_cast<Person*>(entity_x.get())) {
                    const float random_jitter = randSign() * TILESIZE / 2.0f;
                    if (facedir_x & FrontFaceDirection::LEFT) {
                        entity_x->pushed_force.x +=
                            TILESIZE / directional_push_modifier;
                        entity_x->pushed_force.z += random_jitter;
                    }
                    if (facedir_x & FrontFaceDirection::RIGHT) {
                        entity_x->pushed_force.x -=
                            TILESIZE / directional_push_modifier;
                        entity_x->pushed_force.z += random_jitter;
                    }
                }
            }
            if (auto entity_z = collided_entity_z.lock()) {
                if (auto person_ptr_z = dynamic_cast<Person*>(entity_z.get())) {
                    const float random_jitter = randSign() * TILESIZE / 2.0f;
                    if (facedir_z & FrontFaceDirection::FORWARD) {
                        person_ptr_z->pushed_force.x += random_jitter;
                        person_ptr_z->pushed_force.z +=
                            TILESIZE / directional_push_modifier;
                    }
                    if (facedir_z & FrontFaceDirection::BACK) {
                        person_ptr_z->pushed_force.x += random_jitter;
                        person_ptr_z->pushed_force.z -=
                            TILESIZE / directional_push_modifier;
                    }
                }
            }
        }
    }

    std::tuple<int, int> get_face_direction(vec3 new_pos_x, vec3 new_pos_z) {
        int facedir_x = -1;
        int facedir_z = -1;

        vec3 delta_distance_x = new_pos_x - this->raw_position;
        if (delta_distance_x.x > 0) {
            facedir_x = FrontFaceDirection::RIGHT;
        } else if (delta_distance_x.x < 0) {
            facedir_x = FrontFaceDirection::LEFT;
        }

        vec3 delta_distance_z = new_pos_z - this->raw_position;
        if (delta_distance_z.z > 0) {
            facedir_z = FrontFaceDirection::FORWARD;
        } else if (delta_distance_z.z < 0) {
            facedir_z = FrontFaceDirection::BACK;
        }
        return std::make_tuple(facedir_x, facedir_z);
    }

    void update_facing_direction(int facedir_x, int facedir_z) {
        if (facedir_x == -1 && facedir_z == -1) {
            // do nothing
        } else if (facedir_x == -1) {
            this->face_direction = static_cast<FrontFaceDirection>(facedir_z);
        } else if (facedir_z == -1) {
            this->face_direction = static_cast<FrontFaceDirection>(facedir_x);
        } else {
            this->face_direction =
                static_cast<FrontFaceDirection>(facedir_x | facedir_z);
        }
    }

    virtual void update(float dt) override {
        // std::cout << this->raw_position << ";; " << this->position <<
        // std::endl;

        auto new_pos_x = this->update_xaxis_position(dt);
        auto new_pos_z = this->update_zaxis_position(dt);

        auto facedirs = get_face_direction(new_pos_x, new_pos_z);
        int facedir_x = std::get<0>(facedirs);
        int facedir_z = std::get<1>(facedirs);

        update_facing_direction(facedir_x, facedir_z);

        new_pos_x.x += this->pushed_force.x;
        this->pushed_force.x = 0.0f;

        new_pos_z.z += this->pushed_force.z;
        this->pushed_force.z = 0.0f;

        // this->face_direction = FrontFaceDirection::BACK &
        // FrontFaceDirection::LEFT;

        handle_collision(facedir_x, new_pos_x, facedir_z, new_pos_z);

        Entity::update(dt);
    }
};
