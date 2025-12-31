
#pragma once

#include "../bitsery_include.h"
#include "../globals.h"
#include "ah.h"

using afterhours::Entity;
using afterhours::EntityID;
using bitsery::ext::PointerObserver;

struct BaseComponent : public afterhours::BaseComponent {
    BaseComponent() = default;
    // Afterhours' BaseComponent intentionally does not define copy operations.
    // However, PharmaSea components frequently declare destructors, which
    // suppresses implicit move generation. The pooled ComponentStore requires
    // components to be relocatable during pool maintenance (swap-remove, etc.).
    //
    // Since afterhours::BaseComponent is currently an empty tag base, we provide
    // explicit copy operations that reinitialize the base and let derived
    // members copy normally.
    BaseComponent(const BaseComponent&) : afterhours::BaseComponent() {}
    BaseComponent& operator=(const BaseComponent&) { return *this; }
    BaseComponent(BaseComponent&&) noexcept : afterhours::BaseComponent() {}
    BaseComponent& operator=(BaseComponent&&) noexcept { return *this; }
    virtual ~BaseComponent() = default;
    friend bitsery::Access;

    template<typename S>
    void serialize(S&) {}
};
