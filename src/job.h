
#pragma once

#include "engine/assert.h"
#include "external_include.h"
//

struct Entity;

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

struct Job {
    enum struct State {
        Initialize,
        HeadingToStart,
        HeadingToEnd,
        WorkingAtStart,
        WorkingAtEnd,
        Completed
    } state = State::Initialize;

    JobType type;

    vec2 start;
    vec2 end;
    std::optional<vec2> local;

   private:
    int path_size = 0;
    std::deque<vec2> path;

   public:
    [[nodiscard]] bool path_empty() const { return path.empty(); }
    [[nodiscard]] const vec2& path_front() const { return path.front(); }
    [[nodiscard]] const std::deque<vec2>& get_path() const { return path; }
    [[nodiscard]] int p_size() const { return (int) path.size(); }

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

        s.object(start);
        s.object(end);

        s.ext(local, bitsery::ext::StdOptional{});

        s.value4b(path_size);
        s.container(path, path_size, [](S& sv, vec2 pos) { sv.object(pos); });
    }

    static Job* create_job_of_type(const std::shared_ptr<Entity>& entity,
                                   float dt, JobType type);

    void run_job_tick(const std::shared_ptr<Entity>& entity, float dt) {
        state = _run_job_tick(entity, dt);
        return;
    }

    State _run_job_tick(const std::shared_ptr<Entity>& entity, float dt) {
        switch (state) {
            case Job::State::Initialize: {
                return run_state_initialize(entity, dt);
            }
            case Job::State::HeadingToStart: {
                return run_state_heading_to_start(entity, dt);
            }
            case Job::State::WorkingAtStart: {
                return run_state_working_at_start(entity, dt);
            }
            case Job::State::HeadingToEnd: {
                return run_state_heading_to_end(entity, dt);
            }
            case Job::State::WorkingAtEnd: {
                return run_state_working_at_end(entity, dt);
            }
            case Job::State::Completed: {
                return run_state_completed(entity, dt);
            }
        }
    }

    Job() {}

    Job(JobType _type, vec2 _start, vec2 _end)
        : type(_type), start(_start), end(_end) {}

    virtual ~Job() {}

   protected:
    virtual State run_state_initialize(const std::shared_ptr<Entity>&, float) {
        return State::HeadingToStart;
    }

    virtual State run_state_heading_to_start(
        const std::shared_ptr<Entity>& entity, float dt);
    virtual State run_state_heading_to_end(
        const std::shared_ptr<Entity>& entity, float dt);

    virtual State run_state_working_at_start(const std::shared_ptr<Entity>&,
                                             float) {
        return State::HeadingToEnd;
    }
    virtual State run_state_working_at_end(const std::shared_ptr<Entity>&,
                                           float) {
        return State::Completed;
    }

    virtual State run_state_completed(const std::shared_ptr<Entity>&, float) {
        return State::Completed;
    }

   private:
    bool has_local_target() const { return local.has_value(); }

    [[nodiscard]] inline bool is_at_position(
        const std::shared_ptr<Entity>& entity, vec2 position);

    inline void travel_to_position(const std::shared_ptr<Entity>& entity,
                                   float dt, vec2 goal);
};

struct Wandering : public Job {
    Wandering() : Job(JobType::Wandering, vec2{0, 0}, vec2{0, 0}) {}
    Wandering(vec2 _start, vec2 _end) : Job(JobType::Wandering, _start, _end) {}

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Job>{});
    }
};

struct Wait : public Job {
    float timePassedInCurrentState = 0.f;
    float timeToComplete = 1.f;

    Wait() : Job(JobType::Wait, vec2{0, 0}, vec2{0, 0}), timeToComplete(1.f) {}
    Wait(vec2 _start, vec2 _end, float _timeToComplete)
        : Job(JobType::Wait, _start, _end), timeToComplete(_timeToComplete) {}

    virtual State run_state_working_at_end(
        const std::shared_ptr<Entity>& entity, float dt) override;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Job>{});

        s.value4b(timePassedInCurrentState);
        s.value4b(timeToComplete);
    }
};

struct WaitInQueue : public Job {
    WaitInQueue() : Job(JobType::WaitInQueue, vec2{0, 0}, vec2{0, 0}) {}

    std::shared_ptr<Entity> reg;
    int spot_in_line = -1;

    virtual State run_state_initialize(const std::shared_ptr<Entity>& entity,
                                       float dt) override;
    virtual State run_state_working_at_start(
        const std::shared_ptr<Entity>& entity, float dt) override;
    virtual State run_state_working_at_end(
        const std::shared_ptr<Entity>& entity, float dt) override;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Job>{});

        s.value4b(spot_in_line);
        s.ext(reg, bitsery::ext::StdSmartPtr{});
    }
};
