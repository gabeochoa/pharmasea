
#pragma once

#include "../external_include.h"
//
#include "../engine/assert.h"
#include "../entity.h"
#include "../globals.h"
//
#include "../furniture.h"
#include "../person.h"

struct RegisterNextQueuePosition : Person {
    RegisterNextQueuePosition(vec3 p, Color face_color_in, Color base_color_in)
        : Person(p, face_color_in, base_color_in) {}
    RegisterNextQueuePosition(vec2 p, Color face_color_in, Color base_color_in)
        : Person(p, face_color_in, base_color_in) {}
    RegisterNextQueuePosition(vec3 p, Color c) : Person(p, c) {}
    RegisterNextQueuePosition(vec2 p, Color c) : Person(p, c) {}

    virtual bool is_collidable() override { return false; }
};

struct Register : public Furniture {
    std::deque<Person*> ppl_in_line;
    int max_queue_size = 3;
    int next_line_position = 0;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Furniture>{});
        // Only need to serialize things that are needed for render
        // s.value4b(max_queue_size);
        // s.value4b(next_line_position);
        // s.container(ppl_in_line, max_queue_size);
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

    int position_in_line(Person* entity) {
        for (int i = 0; i < (int) ppl_in_line.size(); i++) {
            if (entity->id == ppl_in_line[i]->id) return i;
        }
        log_warn("Cannot find entity {}", entity->id);
        return -1;
    }

    bool can_move_up(Person* entity) {
        return entity->id == ppl_in_line.front()->id;
    }

    void leave_line(Person* entity) {
        int pos = this->position_in_line(entity);
        if (pos == -1) return;
        if (pos == 0) {
            ppl_in_line.pop_front();
            return;
        }
        ppl_in_line.erase(ppl_in_line.begin() + pos);
    }

    bool is_in_line(Person* entity) { return position_in_line(entity) != -1; }
    bool has_space_in_queue() { return next_line_position < max_queue_size; }

    vec2 get_next_queue_position(Person* entity) {
        M_ASSERT(entity, "entity passed to register queue should not be null");
        ppl_in_line.push_back(entity);
        // the place the customers stand is 1 tile infront of the register
        auto front = this->tile_infront((next_line_position + 1) * 2);
        next_line_position++;
        return front;
    }
};
