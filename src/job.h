
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

struct Entity;

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
    std::optional<vec2> local;
    int path_size = 0;
    std::deque<vec2> path;

    std::function<void(Entity*, Job*)> on_cleanup = nullptr;

    std::map<std::string, void*> data;

    // WaitInQueue Job
    int spot_in_line = -1;

    [[nodiscard]] bool path_empty() const { return path.empty(); }
    [[nodiscard]] const vec2& path_front() const { return path.front(); }
    void path_pop_front() {
        path.pop_front();
        path_size = (int) path.size();
    }

    void update_path(const std::deque<vec2>& new_path) {
        path = new_path;
        path_size = (int) path.size();
    }

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.value4b(type);
        s.value4b(state);

        s.value4b(timePassedInCurrentState);
        s.value4b(timeToComplete);

        s.value4b(spot_in_line);

        s.object(start);
        s.object(end);
        s.value4b(path_size);
        s.ext(local, bitsery::ext::StdOptional{});
        s.container(path, path_size, [](S& sv, vec2 pos) { sv.object(pos); });
    }
};
