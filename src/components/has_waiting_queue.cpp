
#include "has_waiting_queue.h"

#include "../engine/log.h"
#include "../entity.h"

HasWaitingQueue& HasWaitingQueue::add_customer(const Entity& customer) {
    log_info("we are adding {} {} to the line in position {}", customer.id,
             customer.name(), next_line_position);
    ppl_in_line[next_line_position] = customer.id;
    next_line_position++;

    VALIDATE(has_matching_person(customer.id) >= 0,
             "Customer should be in line somewhere");
    return *this;
}

void HasWaitingQueue::dump_contents() const {
    log_info("dumping contents of ppl_in_line");
    for (int i = 0; i < max_queue_size; i++) {
        log_info("index: {}, set? {}, id {}", i, has_person_in_position(i),
                 has_person_in_position(i) ? ppl_in_line[i] : -1);
    }
}

bool HasWaitingQueue::matching_id(int id, int i) const {
    return has_person_in_position(i) ? ppl_in_line[i] == id : false;
}

// TODO rename this or fix return type to work in other contexts better
int HasWaitingQueue::has_matching_person(int id) const {
    for (int i = 0; i < max_queue_size; i++) {
        if (matching_id(id, i)) return i;
    }
    log_warn("Cannot find customer {} in line", id);
    return -1;
}
