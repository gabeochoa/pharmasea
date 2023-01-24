
#pragma once

#include "../external_include.h"
//
#include "../components/has_waiting_queue.h"
#include "../engine/assert.h"
#include "../entity.h"
#include "../globals.h"
//
#include "../aiperson.h"
#include "../furniture.h"

struct RegisterNextQueuePosition : Person {
    RegisterNextQueuePosition(vec3 p, Color face_color_in, Color base_color_in)
        : Person(p, face_color_in, base_color_in) {}
    RegisterNextQueuePosition(vec2 p, Color face_color_in, Color base_color_in)
        : Person(p, face_color_in, base_color_in) {}
    RegisterNextQueuePosition(vec3 p, Color c) : Person(p, c) {}
    RegisterNextQueuePosition(vec2 p, Color c) : Person(p, c) {}
};

struct Register : public Furniture {
   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Furniture>{});
        // Only need to serialize things that are needed for render
    }

   public:
    Register() : Furniture() {}
    explicit Register(vec2 pos)
        : Furniture(pos, ui::color::grey, ui::color::grey) {
        update_model();
    }

    void update_model() {
        // TODO add a component for this
        get<ModelRenderer>().update(ModelInfo{
            .model_name = "register",
            .size_scale = 10.f,
            .position_offset = vec3{0, -TILESIZE / 2.f, 0},
        });
    }

    int position_in_line(AIPerson* entity) {
        const auto& ppl_in_line = this->get<HasWaitingQueue>().ppl_in_line;

        for (int i = 0; i < (int) ppl_in_line.size(); i++) {
            if (entity->id == ppl_in_line[i]->id) return i;
        }
        log_warn("Cannot find entity {}", entity->id);
        return -1;
    }

    bool can_move_up(AIPerson* entity) {
        const auto& ppl_in_line = this->get<HasWaitingQueue>().ppl_in_line;
        return entity->id == ppl_in_line.front()->id;
    }

    void leave_line(AIPerson* entity) {
        auto& ppl_in_line = this->get<HasWaitingQueue>().ppl_in_line;
        int pos = this->position_in_line(entity);
        if (pos == -1) return;
        if (pos == 0) {
            ppl_in_line.pop_front();
            return;
        }
        ppl_in_line.erase(ppl_in_line.begin() + pos);
    }

    bool is_in_line(AIPerson* entity) { return position_in_line(entity) != -1; }
    bool has_space_in_queue() {
        HasWaitingQueue& hasWaitingQueue = this->get<HasWaitingQueue>();

        return hasWaitingQueue.next_line_position <
               hasWaitingQueue.max_queue_size;
    }

    vec2 get_next_queue_position(AIPerson* entity) {
        M_ASSERT(entity, "entity passed to register queue should not be null");
        HasWaitingQueue& hasWaitingQueue = this->get<HasWaitingQueue>();
        hasWaitingQueue.ppl_in_line.push_back(entity);
        // the place the customers stand is 1 tile infront of the register
        auto front =
            this->tile_infront((hasWaitingQueue.next_line_position + 1) * 2);
        hasWaitingQueue.next_line_position++;
        return front;
    }
};
