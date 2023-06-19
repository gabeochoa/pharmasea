

#pragma once

#include "../dataclass/ailment.h"
#include "base_component.h"

struct CanHaveAilment : public BaseComponent {
    virtual ~CanHaveAilment() {}

    [[nodiscard]] std::shared_ptr<Ailment> ailment() const {
        return my_ailment;
    }
    [[nodiscard]] bool has_ailment() const { return ailment() != nullptr; }

    void update(std::shared_ptr<Ailment> new_ailment) {
        my_ailment = new_ailment;
    }

   private:
    std::shared_ptr<Ailment> my_ailment;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        // TODO serialize this
        // s.object(ailment);
    }
};
