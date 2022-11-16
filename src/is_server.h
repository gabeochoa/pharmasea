
#pragma once

#include <thread>
//
#include "globals_register.h"

static bool is_server() {
    auto my_thread_id = std::this_thread::get_id();
    auto server_thread_id =
        GLOBALS.get_or_default("server_thread_id", std::thread::id());
    return my_thread_id == server_thread_id;
}
