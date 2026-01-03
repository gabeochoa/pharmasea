
#pragma once

#include "../globals.h"
#include "../zpp_bits_include.h"
#include "ah.h"

using afterhours::Entity;
using afterhours::EntityID;

struct BaseComponent : public afterhours::BaseComponent {
    BaseComponent() = default;
    virtual ~BaseComponent() = default;

   private:
    friend zpp::bits::access;
    constexpr static auto serialize(auto&, auto&) { return std::errc{}; }
};
