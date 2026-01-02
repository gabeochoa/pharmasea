
#pragma once

#include "../bitsery_include.h"
#include "../globals.h"
#include "ah.h"

using afterhours::Entity;
using afterhours::EntityID;

struct BaseComponent : public afterhours::BaseComponent {
    BaseComponent() = default;
    virtual ~BaseComponent() = default;
    friend bitsery::Access;

    template<typename S>
    void serialize(S&) {}
};
