

#pragma once

#include "base_component.h"

struct ShowsProgressBar : public BaseComponent {
    enum Enabled { Always, Planning, InRound } type;

    ShowsProgressBar() : type(Enabled::Always) {}
    explicit ShowsProgressBar(Enabled e) : type(e) {}

    virtual ~ShowsProgressBar() {}

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value4b(type);
    }
};
