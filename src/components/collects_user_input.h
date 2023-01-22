
#pragma once

#include "../item.h"
#include "base_component.h"

struct CollectsUserInput : public BaseComponent {
    virtual ~CollectsUserInput() {}

    // TODO make private at some point
    std::vector<UserInput> inputs;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
    }
};
