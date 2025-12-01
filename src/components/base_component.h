
#pragma once

#include "../bitsery_include.h"
#include "../globals.h"
#include "afterhours/ah.h"

using afterhours::Entity;
using afterhours::EntityID;
using bitsery::ext::PointerObserver;

struct BaseComponent : public afterhours::BaseComponent {
    BaseComponent() = default;
    virtual ~BaseComponent() = default;
    friend bitsery::Access;

    template<typename S>
    void serialize(S&) {}
};
