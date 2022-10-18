
#pragma once

#include "../external_include.h"
//
#include "../assert.h"
#include "../entity.h"
#include "../globals.h"
//
#include "../aiperson.h"
#include "../furniture.h"

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

    int max_queue_size = 3;

    int next_line_position = 0;
    int position_in_line(AIPerson* entity) {
        for (int i = 0; i < ppl_in_line.size(); i++) {
            if (entity->id == ppl_in_line[i]->id) return i;
        }
        std::cout << fmt::format("cant find entity {}", entity->id)
                  << std::endl;
        return -1;
    }
    bool can_move_up(AIPerson* entity) {
        return entity->id == ppl_in_line.front()->id;
    }

    void leave_line(AIPerson* entity) {
        // std::cout << fmt::format("removing entity {}", entity->id) << std::endl;
        int pos = this->position_in_line(entity);
        if (pos == -1) return;
        if (pos == 0) {
            ppl_in_line.pop_front();
            return;
        }
        // std::cout << fmt::format("used line position") << std::endl;
        ppl_in_line.erase(ppl_in_line.begin() + pos);
    }

    bool is_in_line(AIPerson* entity) { return position_in_line(entity) != -1; }
    bool has_space_in_queue() { return next_line_position < max_queue_size; }

    vec2 get_next_queue_position(AIPerson* entity) {
        M_ASSERT(entity, "entity passed to register queue should not be null");
        ppl_in_line.push_back(entity);
        // the place the customers stand is 1 tile infront of the register
        auto front = this->tile_infront((next_line_position + 1) * 2);
        next_line_position++;
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

    virtual bool can_place_item_into() override { return true; }
};
