
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

    friend class cereal::access;
    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseComponent>(this), ghost);
    }
};

CEREAL_REGISTER_TYPE(CanBeGhostPlayer);
