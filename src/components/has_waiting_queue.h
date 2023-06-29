

#pragma once

#include <bitsery/traits/deque.h>

#include <deque>

#include "base_component.h"

constexpr int max_queue_size = 3;

struct HasWaitingQueue : public BaseComponent {
    virtual ~HasWaitingQueue() {}

    [[nodiscard]] std::shared_ptr<Entity> person(int i) {
        return ppl_in_line[i];
    }

    void erase(int index) {
        for (int i = max_queue_size - 1; i > index; i--) {
            ppl_in_line[i - 1] = ppl_in_line[i];
        }
    }

    [[nodiscard]] int get_next_pos() const { return next_line_position; }

    // These impl are in job.cpp
    [[nodiscard]] bool matching_person(int id, int i) const;
    HasWaitingQueue& add_customer(const std::shared_ptr<Entity>& customer);

   private:
    std::array<std::shared_ptr<Entity>, max_queue_size> ppl_in_line;
    int next_line_position = 0;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value4b(next_line_position);

        s.container(ppl_in_line, [](S& sv, std::shared_ptr<Entity> entity) {
            sv.object(entity);
        });
    }
};
