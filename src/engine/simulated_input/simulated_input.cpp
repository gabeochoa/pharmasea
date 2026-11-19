#include "simulated_input.h"

#include "bypass_helper.h"
#include "e2e_manager.h"

namespace simulated_input {

void init() { e2e_manager::init(); }

void update(float dt) {
    bypass_helper::inject_clicks_for_bypass(dt);
    input_injector::update_key_hold(dt);
    e2e_manager::update(dt);
}

void stop() {}

}  // namespace simulated_input
