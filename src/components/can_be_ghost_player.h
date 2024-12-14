
#pragma once

#include "base_component.h"

// TODO is this actually used anymore
struct CanBeGhostPlayer : public BaseComponent {
    virtual ~CanBeGhostPlayer() {}

    [[nodiscard]] bool is_ghost() const { return ghost; }
    [[nodiscard]] bool is_not_ghost() const { return !is_ghost(); }

    void update(bool g) { ghost = g; }

   private:
    bool ghost = false;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.value1b(ghost);
    }
};
