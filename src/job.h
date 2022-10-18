
#pragma once

#include "external_include.h"
//

enum JobType {
    None = 0,
    Wait = 1,

    Wandering,
    EnterStore,
    WaitInQueue,
    AtRegister,
    Paying,
    Leaving,

    MAX_JOB_TYPE,
};

struct AIPerson;

struct Job {
    JobType type;
    // TODO replace these with just an enum?
    bool reached_start = false;
    bool start_completed = false;
    bool reached_end = false;
    bool is_complete = false;

    float timePassedInCurrentState = 0.f;
    float timeToComplete = 1.f;

    vec2 start;
    vec2 end;
    std::deque<vec2> path;
    std::optional<vec2> local;

    std::function<void(AIPerson*, Job*)> on_cleanup = nullptr;

    std::map<std::string, void*> data;

    // WaitInQueue Job
    bool initialized = false;
    int spot_in_line = -1;
};
