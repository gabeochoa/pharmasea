

#pragma once

#include "base_component.h"

#include "../persistent_entity_ref.h"

struct IsPnumaticPipe : public BaseComponent {
    bool recieving = false;
    int item_id = -1;

    PersistentEntityRef paired{};

    [[nodiscard]] bool has_pair() const { return paired.id != entity_id::INVALID; }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.object(paired);

        // s.value4b(item_id);
    }
};
