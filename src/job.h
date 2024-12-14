
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

    WaitInQueue,
    Paying,
    Drinking,
    Mopping,
    Bathroom,
    PlayJukebox,
    Wandering,

    // Not used yet
    EnterStore,
    WaitInQueueForPickup,
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

   private:
    template<class Archive>
    void serialize(Archive& archive) {
        archive(type, state, start, end);
    }

   public:
    Job() {}

    Job(JobType _type, vec2 _start, vec2 _end)
        : type(_type), start(_start), end(_end) {}

    virtual ~Job() {}

    virtual void before_each_job_tick(Entity&, float) {}

    virtual State run_state_initialize(Entity&, float) {
        return State::HeadingToStart;
    }

    virtual State run_state_working_at_start(Entity&, float) {
        return State::HeadingToEnd;
    }
    virtual State run_state_working_at_end(Entity&, float) {
        return State::Completed;
    }

    virtual void on_cleanup() {}

    Entity& get_and_validate_entity(int id);

   private:
    [[nodiscard]] inline bool is_at_position(const Entity& entity,
                                             vec2 position);
};
