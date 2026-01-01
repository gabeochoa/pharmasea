

#pragma once

#include "../engine/log.h"
#include "../job.h"
#include "base_component.h"

struct CanPerformJob : public BaseComponent {
    JobType current = JobType::NoJob;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value4b(current);
        // Note we are choosing not to send this data to the client
        // s.ext(current_job, bitsery::ext::StdSmartPtr{});

        // Only things that need to be rendered, need to be serialized :)
    }
};
