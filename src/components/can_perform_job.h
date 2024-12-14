

#pragma once

#include "../engine/log.h"
#include "../job.h"
#include "base_component.h"

struct CanPerformJob : public BaseComponent {
    virtual ~CanPerformJob() {}

    JobType current = JobType::NoJob;

   private:
    friend class cereal::access;
    template<class Archive>
    void serialize(Archive& archive) {
        archive(
            cereal::base_class<BaseComponent>(this),
            //
            current
            // Note we are choosing not to send this data to the client
            // s.ext(current_job, bitsery::ext::StdSmartPtr{});

            // Only things that need to be rendered, need to be serialized :)
        );
    }
};

CEREAL_REGISTER_TYPE(CanPerformJob);
