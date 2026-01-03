

#pragma once

#include "../engine/log.h"
#include "../job.h"
#include "base_component.h"

struct CanPerformJob : public BaseComponent {
    JobType current = JobType::NoJob;

   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        // Only things that need to be rendered, need to be serialized :)
        return archive(                      //
            static_cast<BaseComponent&>(self), //
            self.current                     //
        );
    }
};
