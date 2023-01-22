

#pragma once

#include "base_component.h"

struct HasWork : public BaseComponent {
    virtual ~HasWork() {}

    // Does this have work to be done?
    [[nodiscard]] bool has_work() const { return true; }

    void update_pct(float pct) { pct_work_complete = pct; }

    // TODO make private
    float pct_work_complete = 0.f;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        //
        s.value4b(pct_work_complete);
    }
};
