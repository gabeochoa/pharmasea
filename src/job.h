
#pragma once

#include "external_include.h"
//

enum JobType {
    None = 0,

    Wandering,
    EnterStore,
    WaitInQueue,
    AtRegister,
    Paying,
    Leaving,

    MAX_JOB_TYPE,
};

struct Job {
    JobType type;
    // TODO replace these with just an enum?
    bool reached_start = false;
    bool start_completed = false;
    bool reached_end = false;
    bool is_complete = false;

    vec2 start;
    vec2 end;
    std::deque<vec2> path;
    std::optional<vec2> local;

    std::map<std::string, std::shared_ptr<void*>> data;
};
