

#pragma once

#include "../engine/log.h"
#include "../job.h"
#include "base_component.h"

struct CanPerformJob : public BaseComponent {
    virtual ~CanPerformJob() {}

    [[nodiscard]] bool has_job() const { return current_job != nullptr; }
    [[nodiscard]] bool needs_job() const { return !has_job(); }

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

    void cleanup_if_completed() {
        if (needs_job()) return;
        if (current_job->state != Job::State::Completed) return;

        // log_info("cleanup_if_completed : {} job completed",
        // magic_enum::enum_name(current_job->type));

        if (personal_queue.empty()) {
            current_job = nullptr;
            return;
        }

        auto& prev_job = personal_queue.top();
        current_job = prev_job;
        personal_queue.pop();

        // log_info("going to personal queue next {}",
        // magic_enum::enum_name(current_job->type));
    }

    void run_tick(const std::shared_ptr<Entity>& entity, float dt) {
        if (needs_job()) return;
        current_job->run_job_tick(entity, dt);
    }

    void for_each_path_location(std::function<void(vec2)> cb) const {
        if (needs_job()) return;

        for (auto location : job().get_path()) {
            cb(location);
        }
    }

    vec2 job_start() {
        if (needs_job()) return vec2{0, 0};
        return job().start;
    }

   private:
    [[nodiscard]] const Job& job() const { return *current_job; }

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
