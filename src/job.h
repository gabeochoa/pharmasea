
#pragma once

#include "engine/assert.h"
#include "engine/log.h"
#include "external_include.h"
//

using EntityID = int;
struct Entity;

enum JobType {
    NoJob = 0,
    Wait = 1,

    Wandering,
    EnterStore,
    WaitInQueue,
    WaitInQueueForPickup,
    Paying,
    Leaving,
    Drinking,
    Mopping,
    Bathroom,

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
    [[nodiscard]] vec2 path_index(int index) const { return path[index]; }

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

    static Job* create_job_of_type(vec2, vec2, JobType type);

    void run_job_tick(Entity& entity, float dt) {
        state = _run_job_tick(entity, dt);
    }

    Job() {}

    Job(JobType _type, vec2 _start, vec2 _end)
        : type(_type), start(_start), end(_end) {}

    virtual ~Job() {}

   protected:
    virtual State run_state_initialize(Entity&, float) {
        return State::HeadingToStart;
    }

    virtual State run_state_heading_to_start(Entity& entity, float dt);
    virtual State run_state_heading_to_end(Entity& entity, float dt);

    virtual State run_state_working_at_start(Entity&, float) {
        return State::HeadingToEnd;
    }
    virtual State run_state_working_at_end(Entity&, float) {
        return State::Completed;
    }

    virtual void on_cleanup() {}

    virtual void before_each_job_tick(Entity&, float) {}

    bool has_local_target() const { return local.has_value(); }

    Entity& get_and_validate_entity(int id);

   private:
    State _run_job_tick(Entity& entity, float dt) {
        before_each_job_tick(entity, dt);
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
                // We dont run anything because completed is only used for
                // marking cleanup
                return state;
            }
        }
        return state;
    }

    [[nodiscard]] inline bool is_at_position(const Entity& entity,
                                             vec2 position);

    inline void travel_to_position(Entity& entity, float dt, vec2 goal);
};

struct WanderingJob : public Job {
    WanderingJob() : Job(JobType::Wandering, vec2{0, 0}, vec2{0, 0}) {}
    WanderingJob(vec2 _start, vec2 _end)
        : Job(JobType::Wandering, _start, _end) {}

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Job>{});
    }
};

struct WaitJob : public Job {
    float timePassedInCurrentState = 0.f;
    float timeToComplete = 1.f;

    WaitJob()
        : Job(JobType::Wait, vec2{0, 0}, vec2{0, 0}), timeToComplete(1.f) {}
    WaitJob(vec2 _start, vec2 _end, float _timeToComplete)
        : Job(JobType::Wait, _start, _end), timeToComplete(_timeToComplete) {}

    virtual State run_state_working_at_end(Entity& entity, float dt) override;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Job>{});

        s.value4b(timePassedInCurrentState);
        s.value4b(timeToComplete);
    }
};

struct WaitInQueueJob : public Job {
    WaitInQueueJob() : Job(JobType::WaitInQueue, vec2{0, 0}, vec2{0, 0}) {}

    EntityID reg_id = -1;
    int spot_in_line = -1;

    virtual State run_state_initialize(Entity& entity, float dt) override;
    virtual State run_state_working_at_start(Entity& entity, float dt) override;
    virtual State run_state_working_at_end(Entity& entity, float dt) override;

    virtual void before_each_job_tick(Entity&, float) override;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Job>{});

        s.value4b(spot_in_line);
        s.value4b(reg_id);
    }
};

struct LeavingJob : public Job {
    LeavingJob() : Job(JobType::Leaving, vec2{0, 0}, vec2{0, 0}) {}
    LeavingJob(vec2 _start, vec2 _end) : Job(JobType::Leaving, _start, _end) {}

    virtual State run_state_working_at_end(Entity& entity, float dt) override;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Job>{});
    }
};

struct DrinkingJob : public Job {
    float timePassedInCurrentState = 0.f;
    float timeToComplete = 1.f;

    DrinkingJob()
        : Job(JobType::Drinking, vec2{0, 0}, vec2{0, 0}), timeToComplete(1.f) {}
    DrinkingJob(vec2 _start, vec2 _end, float ttc)
        : Job(JobType::Drinking, _start, _end), timeToComplete(ttc) {}

    virtual State run_state_working_at_end(Entity& entity, float dt) override;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Job>{});

        s.value4b(timePassedInCurrentState);
        s.value4b(timeToComplete);
    }
};

struct BathroomJob : public Job {
    float timePassedInCurrentState = 0.f;
    float timeToComplete = 1.f;
    EntityID toilet_id = -1;

    BathroomJob()
        : Job(JobType::Bathroom, vec2{0, 0}, vec2{0, 0}), timeToComplete(1.f) {}
    BathroomJob(vec2 _start, vec2 _end, float ttc)
        : Job(JobType::Bathroom, _start, _end), timeToComplete(ttc) {}

    virtual State run_state_initialize(Entity& entity, float dt) override;
    virtual State run_state_working_at_start(Entity& entity, float dt) override;
    virtual State run_state_working_at_end(Entity& entity, float dt) override;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Job>{});

        s.value4b(timePassedInCurrentState);
        s.value4b(timeToComplete);
    }
};

struct MoppingJob : public Job {
    EntityID vom_id = -1;

    float timePassedInCurrentState = 0.f;
    float timeToComplete = 1.f;

    MoppingJob()
        : Job(JobType::Mopping, vec2{0, 0}, vec2{0, 0}), timeToComplete(1.f) {}
    MoppingJob(vec2 _start, vec2 _end, float ttc)
        : Job(JobType::Mopping, _start, _end), timeToComplete(ttc) {}

    virtual State run_state_initialize(Entity& entity, float dt) override;
    virtual State run_state_working_at_start(Entity& entity, float dt) override;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Job>{});

        s.value4b(timePassedInCurrentState);
        s.value4b(timeToComplete);
    }
};
