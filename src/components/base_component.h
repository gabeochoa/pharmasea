
#pragma once

#include "../bitsery_include.h"
#include "../globals.h"
#include "ah.h"

using afterhours::Entity;
using afterhours::EntityID;
using bitsery::ext::PointerObserver;

struct BaseComponent : public afterhours::BaseComponent {
    BaseComponent() = default;
    BaseComponent(const BaseComponent&) = delete;
    BaseComponent& operator=(const BaseComponent&) = delete;
    BaseComponent(BaseComponent&&) noexcept = default;
    BaseComponent& operator=(BaseComponent&&) noexcept = delete;
    virtual ~BaseComponent() = default;
    friend bitsery::Access;

    template<typename S>
    void serialize(S&) {}
};
