

#pragma once

#include "../job.h"
#include "base_component.h"

struct CanPerformJob : public BaseComponent {
    virtual ~CanPerformJob() {}

    [[nodiscard]] bool has_job() const { return current_job != nullptr; }
    [[nodiscard]] std::shared_ptr<Job> job() const { return current_job; }
    [[nodiscard]] std::stack<std::shared_ptr<Job>> job_queue() const {
        return personal_queue;
    }

   private:
    std::stack<std::shared_ptr<Job>> personal_queue;
    std::shared_ptr<Job> current_job;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        // Only things that need to be rendered, need to be serialized :)
    }
};
