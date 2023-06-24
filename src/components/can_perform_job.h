

#pragma once

#include "../job.h"
#include "base_component.h"

struct CanPerformJob : public BaseComponent {
    virtual ~CanPerformJob() {}

    [[nodiscard]] bool has_job() const { return current_job != nullptr; }
    [[nodiscard]] bool needs_job() const { return !has_job(); }
    [[nodiscard]] const Job& job() const { return *current_job; }
    [[nodiscard]] std::shared_ptr<Job> mutable_job() { return current_job; }
    [[nodiscard]] std::stack<std::shared_ptr<Job>>& job_queue() {
        return personal_queue;
    }

    void update(JobType starting_jt, JobType idle_jt) {
        starting_job_type = starting_jt;
        idle_job_type = idle_jt;
    }

    void update(Job* job) { current_job.reset(job); }

    [[nodiscard]] JobType get_next_job_type() {
        if (worked_before) return idle_job_type;
        worked_before = true;
        return starting_job_type;
    }

    void push_and_reset(Job* job) {
        personal_queue.push(current_job);
        current_job.reset(job);
    }

    void push_onto_queue(std::shared_ptr<Job> job) { personal_queue.push(job); }

    // Job stuff
    [[nodiscard]] bool has_local_target() const {
        return has_job() && job().local.has_value();
    }
    [[nodiscard]] vec2 local_target() const {
        return current_job->local.value();
    }

    void update_job_state(Job::State new_state) {
        current_job->state = new_state;
    }

    void grab_job_local_target() {
        // Grab next local point to go to
        current_job->local = current_job->path_front();
        current_job->path_pop_front();
        return;
    }

   private:
    JobType starting_job_type;
    JobType idle_job_type;
    bool worked_before = false;

    std::stack<std::shared_ptr<Job>> personal_queue;
    std::shared_ptr<Job> current_job;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.ext(current_job, bitsery::ext::StdSmartPtr{});
        // Only things that need to be rendered, need to be serialized :)
    }
};
