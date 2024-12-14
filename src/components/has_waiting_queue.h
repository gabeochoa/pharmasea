

#pragma once

#include "base_component.h"

using EntityID = int;

struct HasWaitingQueue : public BaseComponent {
    static const int max_queue_size = 3;

    virtual ~HasWaitingQueue() {}

    [[nodiscard]] int num_in_queue() const {
        return max_queue_size - (max_queue_size - next_line_position);
    }
    [[nodiscard]] bool is_full() const { return !has_space(); }
    [[nodiscard]] bool has_space() const {
        return next_line_position < max_queue_size;
    }
    [[nodiscard]] EntityID person(int i) const { return ppl_in_line[i]; }
    [[nodiscard]] EntityID person(size_t i) const { return ppl_in_line[i]; }

    [[nodiscard]] bool has_person_in_position(size_t i) const {
        return (ppl_in_line[i] != -1);
    }

    void clear() {
        for (int i = 0; i < max_queue_size; ++i) {
            ppl_in_line[i] = -1;
        }
        next_line_position = 0;
    }

    void erase(int index) {
        for (int i = index; i < max_queue_size - 1; ++i) {
            ppl_in_line[i] = ppl_in_line[i + 1];
        }
        ppl_in_line[max_queue_size - 1] = {};

        next_line_position--;
    }

    [[nodiscard]] int get_next_pos() const { return next_line_position; }

    // These impl are in job.cpp
    [[nodiscard]] bool matching_id(int id, int i) const;
    [[nodiscard]] bool has_matching_person(int id) const;
    [[nodiscard]] int get_customer_position(int id) const;
    HasWaitingQueue& add_customer(const Entity& customer);

   private:
    std::array<EntityID, max_queue_size> ppl_in_line;
    int next_line_position = 0;

    // These impl are in job.cpp
    void dump_contents() const;
    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseComponent>(this),
                //
                next_line_position, ppl_in_line);
    }
};

CEREAL_REGISTER_TYPE(HasWaitingQueue);
