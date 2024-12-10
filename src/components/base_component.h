
#pragma once

#include "../bitsery_include.h"
#include "../globals.h"
#include "afterhours/ah.h"
#include "afterhours/src/base_component.h"

using afterhours::Entity;
using afterhours::EntityID;
using bitsery::ext::PointerObserver;

using afterhours::BaseComponent;

namespace bitsery {
template<typename S>
void serialize(S& s, afterhours::BaseComponent& component) {
    s.ext(component.parent, PointerObserver{});
}

}  // namespace bitsery
