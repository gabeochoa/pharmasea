
#pragma once

#include "external_include.h"
//

enum JobType {
    None = 0,
    Wait = 1,

    Wandering,
    EnterStore,
    WaitInQueue,
    WaitInQueueForPickup,
    Paying,
    Leaving,

    MAX_JOB_TYPE,
};

struct AIPerson;

struct Job {
    JobType type;

    enum struct State {
        Initialize,
        HeadingToStart,
        HeadingToEnd,
        WorkingAtStart,
        WorkingAtEnd,
        Completed
    } state;

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
