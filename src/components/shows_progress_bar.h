

#pragma once

#include "base_component.h"

struct ShowsProgressBar : public BaseComponent {
    virtual ~ShowsProgressBar() {}

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
    }
};
