

#pragma once

#include "base_component.h"

struct IsPnumaticPipe : public BaseComponent {
    bool item_changed = false;
    int item_id = -1;

    int paired_id = -1;

    virtual ~IsPnumaticPipe() {}

    [[nodiscard]] bool has_pair() const { return paired_id != -1; }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value4b(paired_id);
        s.value4b(item_id);
        s.value1b(item_changed);
    }
};
