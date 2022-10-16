
#pragma once

#include "external_include.h"
//
#include "entity.h"
#include "entityhelper.h"
//
#include "keymap.h"

struct Person : public Entity {
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

    virtual void update(float dt) override {
        // std::cout << this->raw_position << ";; " << this->position <<
        // std::endl;

        auto new_pos_x = this->update_xaxis_position(dt);
        auto new_pos_z = this->update_zaxis_position(dt);

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

        if (facedir_x == -1 && facedir_z == -1) {
            // do nothing
        } else if (facedir_x == -1) {
            this->face_direction = static_cast<FrontFaceDirection>(facedir_z);
        } else if (facedir_x == FrontFaceDirection::RIGHT) {
            this->face_direction = FrontFaceDirection::RIGHT;
            if (facedir_z == FrontFaceDirection::BACK) {
                this->face_direction = FrontFaceDirection::SE;
            }
            if (facedir_z == FrontFaceDirection::FORWARD) {
                this->face_direction = FrontFaceDirection::NE;
            }
        } else if (facedir_x == FrontFaceDirection::LEFT) {
            this->face_direction = FrontFaceDirection::LEFT;
            if (facedir_z == FrontFaceDirection::BACK) {
                this->face_direction = FrontFaceDirection::SW;
            }
            if (facedir_z == FrontFaceDirection::FORWARD) {
                this->face_direction = FrontFaceDirection::NW;
            }
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
