

#pragma once

#include <deque>

#include "../aiperson.h"
#include "base_component.h"

struct HasWaitingQueue : public BaseComponent {
    virtual ~HasWaitingQueue() {}

    std::deque<AIPerson*> ppl_in_line;
    int max_queue_size = 3;
    int next_line_position = 0;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value4b(max_queue_size);
        s.value4b(next_line_position);
        s.container(ppl_in_line, max_queue_size);
    }
};
