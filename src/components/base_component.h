
#pragma once

#include "../bitsery_include.h"
#include "../globals.h"
#include "ah.h"

using afterhours::Entity;
using afterhours::EntityID;
using bitsery::ext::PointerObserver;

struct BaseComponent : public afterhours::BaseComponent {
    BaseComponent() = default;
    // NOTE: afterhours::BaseComponent is intentionally non-copyable, but PharmaSea
    // components need to be storable in Afterhours' pooled component storage.
    // We provide a trivial copy operation (base has no state) so components with
    // user-declared destructors remain pool-compatible.
    BaseComponent(const BaseComponent&) : afterhours::BaseComponent{} {}
    BaseComponent& operator=(const BaseComponent&) { return *this; }
    BaseComponent(BaseComponent&&) noexcept = default;
    BaseComponent& operator=(BaseComponent&&) noexcept = delete;
    virtual ~BaseComponent() = default;
    friend bitsery::Access;

    template<typename S>
    void serialize(S&) {}
};
