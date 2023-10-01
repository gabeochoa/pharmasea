
#pragma once

#include "../engine/log.h"
#include "base_component.h"

struct IsFloorMarker : public BaseComponent {
    enum Type {
        Unset,
        Planning_SpawnArea,
        Planning_TrashArea,
    } type = Unset;

    explicit IsFloorMarker(Type type) : type(type) {}
    IsFloorMarker() : IsFloorMarker(Unset) {}

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.value4b(type);
    }
};
