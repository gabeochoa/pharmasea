

#pragma once

#include "base_component.h"

constexpr int MAX_QUEUE_SIZE = 3;

struct HasWaitingQueue : public BaseComponent {
    virtual ~HasWaitingQueue() {}

    [[nodiscard]] bool is_full() const { return !has_space(); }
    [[nodiscard]] bool has_space() const {
        return next_line_position < MAX_QUEUE_SIZE;
    }
    [[nodiscard]] OptEntity person(int i) {
        // return ppl_in_line[i];
        // TODO entity helper again
        return {};
    }

    void erase(int index) {
        for (std::size_t i = index; i < MAX_QUEUE_SIZE - 1; ++i) {
            ppl_in_line[i] = ppl_in_line[i + 1];
        }
        ppl_in_line[MAX_QUEUE_SIZE - 1] = {};

        next_line_position--;
    }

    [[nodiscard]] int get_next_pos() const { return next_line_position; }

    // These impl are in job.cpp
    [[nodiscard]] bool matching_id(int id, int i) const;
    [[nodiscard]] bool has_matching_person(int id) const;
    HasWaitingQueue& add_customer(Entity& customer);

   private:
    // TODO why do i need this struct
    // post an issue with a smaller case
    struct ID {
        int id;

        operator int() const { return id; }

       private:
        friend bitsery::Access;
        template<typename S>
        void serialize(S& s) {
            s.value4b(id);
        }
    };

    std::array<ID, MAX_QUEUE_SIZE> ppl_in_line;
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
