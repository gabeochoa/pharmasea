
#pragma once

#include "../external_include.h"
//
#include "../entity.h"
#include "../globals.h"
//
#include "../furniture.h"
#include "../aiperson.h"

struct RegisterNextQueuePosition : TargetCube {
    RegisterNextQueuePosition(vec3 p, Color face_color_in, Color base_color_in)
        : TargetCube(p, face_color_in, base_color_in) {}
    RegisterNextQueuePosition(vec2 p, Color face_color_in, Color base_color_in)
        : TargetCube(p, face_color_in, base_color_in) {}
    RegisterNextQueuePosition(vec3 p, Color c) : TargetCube(p, c) {}
    RegisterNextQueuePosition(vec2 p, Color c) : TargetCube(p, c) {}
};

struct Register : public Furniture {
    Register(vec2 pos) : Furniture(pos, BLACK, DARKGRAY) {}

    std::deque<AIPerson*> ppl_in_line;

    int next_line_position = 1;

    bool is_in_line(std::shared_ptr<AIPerson> entity) {
        if (entity == nullptr) return false;
        for (auto e : ppl_in_line) {
            if (entity->id == e->id) return true;
        }
        return false;
    }

    vec2 get_next_queue_position(AIPerson* entity) {
        if (entity == nullptr) {
            return vec2{-1, -1};
        }
        ppl_in_line.push_back(entity);
        auto front = this->tile_infront(next_line_position++);
        return front;
    }

    virtual void render() const override {
        Furniture::render();

        const float box_size = TILESIZE / 10.f;
        for (auto entity : this->ppl_in_line) {
            DrawCube(entity->snap_position(), box_size, box_size, box_size,
                     PINK);
        }
    }

    virtual bool can_rotate() override { return true; }

    virtual bool can_be_picked_up() override { return true; }
};
