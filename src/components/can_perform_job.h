

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
    }
};
