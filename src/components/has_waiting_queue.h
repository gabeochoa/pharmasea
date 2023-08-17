

#pragma once

#include "base_component.h"

constexpr int max_queue_size = 3;

struct HasWaitingQueue : public BaseComponent {
    virtual ~HasWaitingQueue() {}

    [[nodiscard]] size_t num_in_queue() const {
        return max_queue_size - next_line_position;
    }
    [[nodiscard]] bool is_full() const { return !has_space(); }
    [[nodiscard]] bool has_space() const {
        return next_line_position < max_queue_size;
    }
    [[nodiscard]] std::shared_ptr<Entity> person(int i) {
        return ppl_in_line[i];
    }

    [[nodiscard]] const std::shared_ptr<Entity> person(size_t i) const {
        return ppl_in_line[i];
    }

    void erase(int index) {
        for (std::size_t i = index; i < max_queue_size - 1; ++i) {
            ppl_in_line[i] = ppl_in_line[i + 1];
        }
        ppl_in_line[max_queue_size - 1] = {};

        next_line_position--;
    }

    [[nodiscard]] int get_next_pos() const { return next_line_position; }

    // These impl are in job.cpp
    [[nodiscard]] bool matching_id(int id, int i) const;
    [[nodiscard]] int has_matching_person(int id) const;
    HasWaitingQueue& add_customer(const std::shared_ptr<Entity>& customer);

   private:
    std::array<std::shared_ptr<Entity>, max_queue_size> ppl_in_line;
    int next_line_position = 0;

    // These impl are in job.cpp
    void dump_contents() const;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value4b(next_line_position);
        s.container(ppl_in_line);
    }
};
