
#include "has_waiting_queue.h"

#include "../engine/assert.h"
#include "../engine/log.h"
#include "../entity.h"
#include "../entity_type.h"

HasWaitingQueue& HasWaitingQueue::add_customer(const Entity& customer) {
    log_info("we are adding {} {} to the line in position {}", customer.id,
             str(get_entity_type(customer)), next_line_position);
    ppl_in_line[next_line_position].set_id(customer.id);
    next_line_position++;

    VALIDATE(has_matching_person(customer.id) >= 0,
             "Customer should be in line somewhere");
    return *this;
}

void HasWaitingQueue::dump_contents() const {
    log_info("dumping contents of ppl_in_line");
    for (int i = 0; i < max_queue_size; i++) {
        log_info("index: {}, set? {}, id {}", i, has_person_in_position(i),
                 has_person_in_position(i) ? ppl_in_line[i].id : -1);
    }
}

bool HasWaitingQueue::matching_id(int id, int i) const {
    return has_person_in_position(i) ? ppl_in_line[i].id == id : false;
}

int HasWaitingQueue::get_customer_position(int id) const {
    for (int i = 0; i < max_queue_size; i++) {
        if (matching_id(id, i)) return i;
    }
    log_warn("Cannot find customer {} in line", id);
    return -1;
}

bool HasWaitingQueue::has_matching_person(int id) const {
    return get_customer_position(id) != -1;
}
